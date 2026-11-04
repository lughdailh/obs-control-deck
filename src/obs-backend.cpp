#include "obs-backend.hpp"
#include "dsk-control.hpp"
#include "deck-widget.hpp"
#include "obs-websocket-api.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QSaveFile>
#include <obs-module.h>
#include <QThread>
#include <QTimer>
#include <algorithm>
#include <memory>

namespace deck {
namespace {
using Source = std::unique_ptr<obs_source_t, decltype(&obs_source_release)>;
Source owned(obs_source_t *s)
{
	return Source(s, obs_source_release);
}
QString text(const char *s)
{
	return QString::fromUtf8(s ? s : "");
}
QString uuid(obs_source_t *s)
{
	return s ? text(obs_source_get_uuid(s)) : QString{};
}
QJsonArray hotkeyJson(obs_hotkey_id id)
{
	auto *keys = obs_hotkey_save(id);
	auto *data = obs_data_create();
	obs_data_set_array(data, "keys", keys);
	auto result = QJsonDocument::fromJson(obs_data_get_json(data)).object()["keys"].toArray();
	obs_data_array_release(keys);
	obs_data_release(data);
	return result;
}
void loadHotkey(obs_hotkey_id id, const QJsonArray &keys)
{
	auto *data = obs_data_create_from_json(
		QJsonDocument(QJsonObject{{"keys", keys}}).toJson(QJsonDocument::Compact).constData());
	auto *array = obs_data_get_array(data, "keys");
	obs_hotkey_load(id, array);
	obs_data_array_release(array);
	obs_data_release(data);
}
QString hotkeyPath()
{
	char *path = obs_module_config_path("hotkeys.json");
	const QString result = text(path);
	bfree(path);
	return result;
}
QJsonObject dskRequest(const QString &type, const QJsonObject &data, QString &error)
{
	auto request = obs_data_create();
	obs_data_set_string(request, "vendorName", "downstream-keyer");
	obs_data_set_string(request, "requestType", type.toUtf8().constData());
	auto payload = obs_data_create_from_json(QJsonDocument(data).toJson(QJsonDocument::Compact).constData());
	obs_data_set_obj(request, "requestData", payload);
	obs_data_release(payload);
	auto *response = obs_websocket_call_request("CallVendorRequest", request);
	obs_data_release(request);
	if (!response) {
		error = "Interfície local d’obs-websocket no disponible.";
		return {};
	}
	QJsonObject result;
	if (response->status_code != 100)
		error = "DSK no disponible: " + text(response->comment);
	else {
		result = QJsonDocument::fromJson(response->response_data ? QByteArray(response->response_data)
									 : QByteArray{})
				 .object()["responseData"]
				 .toObject();
		if (result.contains("success") && !result["success"].toBool())
			error = "DSK: " + result["error"].toString("acció rebutjada");
	}
	obs_websocket_request_response_free(response);
	return result;
}
Source findTransition(const QString &name)
{
	obs_frontend_source_list list = {};
	obs_frontend_get_transitions(&list);
	obs_source_t *found = nullptr;
	for (size_t i = 0; i < list.sources.num; ++i) {
		auto *s = list.sources.array[i];
		if ((name == "@cut" && QString::fromUtf8(obs_source_get_id(s)) == "cut_transition") ||
		    text(obs_source_get_name(s)) == name) {
			found = obs_source_get_ref(s);
			break;
		}
	}
	obs_frontend_source_list_free(&list);
	return owned(found);
}
} // namespace
ObsBackend::ObsBackend()
{
	obs_frontend_add_event_callback(eventCallback, this);
	obs_frontend_add_save_callback(saveCallback, this);
	toggleHotkey_ = obs_hotkey_register_frontend("obs-control-deck.toggle", "Botonera: Mostrar/amagar la botonera",
						     toggleHotkey, this);
	QFile file(hotkeyPath());
	if (file.open(QIODevice::ReadOnly))
		loadHotkey(toggleHotkey_, QJsonDocument::fromJson(file.readAll()).object()["toggle"].toArray());
}
ObsBackend::~ObsBackend()
{
	saveToggleHotkey();
	for (const auto &entry : bindings_)
		obs_hotkey_unregister(entry.second->hotkey);
	obs_hotkey_unregister(toggleHotkey_);
	obs_frontend_remove_event_callback(eventCallback, this);
	obs_frontend_remove_save_callback(saveCallback, this);
}
void ObsBackend::attach(DeckWidget *w)
{
	widget_ = w;
	w->load(config_);
}
void ObsBackend::save(const Config &c)
{
	if (!ready_ || shuttingDown_)
		return;
	config_ = portableConfig(c);
	syncHotkeys();
	obs_frontend_save();
}
void ObsBackend::replace(const Config &c)
{
	if (!ready_ || shuttingDown_)
		return;
	config_ = c;
	rejectedConfig_.clear();
	syncHotkeys();
	obs_frontend_save();
}
Config ObsBackend::portableConfig(const Config &c)
{
	Config result = c;
	result.hotkeys = c.hotkeys;
	for (const auto &entry : bindings_)
		result.hotkeys.insert(entry.first, hotkeyJson(entry.second->hotkey));
	return result;
}
void ObsBackend::syncHotkeys()
{
	for (const auto &entry : bindings_)
		obs_hotkey_unregister(entry.second->hotkey);
	bindings_.clear();
	auto actions = config_.buttons;
	actions += config_.controlActions(snapshot().transitions);
	for (const auto &b : actions) {
		if (b.type == "empty")
			continue;
		auto binding = std::make_unique<Binding>();
		binding->owner = this;
		binding->id = b.id;
		const auto name = ("obs-control-deck.button." + b.id).toUtf8();
		const auto description = ("Botonera: " + b.label + " [" + b.type + "]").toUtf8();
		binding->hotkey = obs_hotkey_register_frontend(name.constData(), description.constData(), buttonHotkey,
							       binding.get());
		loadHotkey(binding->hotkey, config_.hotkeys[b.id].toArray());
		bindings_.emplace(b.id, std::move(binding));
	}
}
void ObsBackend::buttonHotkey(void *ptr, obs_hotkey_id, obs_hotkey_t *, bool pressed)
{
	if (!pressed)
		return;
	auto *binding = static_cast<Binding *>(ptr);
	const auto widget = binding->owner->widget_;
	const auto id = binding->id;
	if (widget)
		QMetaObject::invokeMethod(
			widget,
			[widget, id] {
				if (widget)
					widget->activateButton(id);
			},
			Qt::QueuedConnection);
}
void ObsBackend::toggleHotkey(void *ptr, obs_hotkey_id, obs_hotkey_t *, bool pressed)
{
	if (!pressed)
		return;
	const auto widget = static_cast<ObsBackend *>(ptr)->widget_;
	if (widget)
		QMetaObject::invokeMethod(
			widget,
			[widget] {
				if (!widget)
					return;
				auto *dock = qobject_cast<QDockWidget *>(widget->parentWidget());
				if (dock) {
					const bool show = !dock->isVisible();
					dock->setVisible(show);
					if (show)
						dock->raise();
				}
			},
			Qt::QueuedConnection);
}
void ObsBackend::saveToggleHotkey()
{
	const auto path = hotkeyPath();
	if (path.isEmpty())
		return;
	QDir().mkpath(QFileInfo(path).absolutePath());
	QSaveFile file(path);
	if (file.open(QIODevice::WriteOnly)) {
		file.write(QJsonDocument(QJsonObject{{"toggle", hotkeyJson(toggleHotkey_)}}).toJson());
		file.commit();
	}
}
void ObsBackend::saveCallback(obs_data_t *data, bool saving, void *ptr)
{
	auto *self = static_cast<ObsBackend *>(ptr);
	if (saving) {
		self->config_ = self->portableConfig(self->config_);
		self->saveToggleHotkey();
		const auto raw = self->rejectedConfig_.isEmpty()
					 ? QJsonDocument(self->config_.json()).toJson(QJsonDocument::Compact)
					 : self->rejectedConfig_;
		obs_data_set_string(data, "obs-control-deck", raw.constData());
		return;
	}
	Config config;
	QString error;
	const QByteArray raw = obs_data_get_string(data, "obs-control-deck");
	self->rejectedConfig_.clear();
	if (!raw.isEmpty() && !Config::parse(QJsonDocument::fromJson(raw).object(), config, error)) {
		blog(LOG_WARNING, "[obs-control-deck] Cannot load configuration: %s", error.toUtf8().constData());
		self->rejectedConfig_ = raw;
	}
	self->config_ = config;
	self->syncHotkeys();
	if (self->widget_)
		self->widget_->load(config);
}
void ObsBackend::eventCallback(enum obs_frontend_event e, void *ptr)
{
	auto *self = static_cast<ObsBackend *>(ptr);
	// Frontend lifecycle callbacks run on OBS's UI thread.
	self->event(e);
}
void ObsBackend::event(enum obs_frontend_event e)
{
	switch (e) {
	case OBS_FRONTEND_EVENT_STREAMING_STARTING:
	case OBS_FRONTEND_EVENT_RECORDING_STARTING:
		if (!shuttingDown_) {
			config_.locked = true;
			if (widget_) widget_->outputStarting();
		}
		break;
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP:
		ready_ = false;
		restoreTransition_.clear();
		break;
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
		ready_ = true;
		break;
	case OBS_FRONTEND_EVENT_EXIT:
		shuttingDown_ = true;
		ready_ = false;
		break;
	case OBS_FRONTEND_EVENT_TRANSITION_STOPPED:
		if (!restoreTransition_.isEmpty()) {
			const auto name = restoreTransition_;
			const int duration = restoreDuration_;
			restoreTransition_.clear();
			auto current = owned(obs_frontend_get_current_transition());
			if (current && QString::fromUtf8(obs_source_get_id(current.get())) == "cut_transition") {
				auto prior = findTransition(name);
				if (prior) {
					obs_frontend_set_current_transition(prior.get());
					obs_frontend_set_transition_duration(duration);
				}
			}
		}
		break;
	default:
		break;
	}
	// Polling coalesces rapid frontend signals without re-entering OBS while it mutates state.
}
Snapshot ObsBackend::snapshot()
{
	Snapshot s;
	s.ready = ready_ && !shuttingDown_ && rejectedConfig_.isEmpty();
	if (!rejectedConfig_.isEmpty())
		s.blockedReason =
			"Configuració de botonera no compatible. S’ha conservat l’original sense modificar-lo.";
	if (!s.ready)
		return s;
	s.studio = obs_frontend_preview_program_mode_active();
	auto program = owned(obs_frontend_get_current_scene());
	auto preview = owned(obs_frontend_get_current_preview_scene());
	s.program = uuid(program.get());
	s.preview = uuid(preview.get());
	obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);
	for (size_t i = 0; i < scenes.sources.num; ++i)
		s.scenes.push_back({uuid(scenes.sources.array[i]), text(obs_source_get_name(scenes.sources.array[i]))});
	obs_frontend_source_list_free(&scenes);
	obs_frontend_source_list transitions = {};
	obs_frontend_get_transitions(&transitions);
	for (size_t i = 0; i < transitions.sources.num; ++i) {
		auto *source = transitions.sources.array[i];
		const auto name = text(obs_source_get_name(source));
		s.transitions << name;
		s.transitionKinds.insert(name, text(obs_source_get_id(source)));
	}
	obs_frontend_source_list_free(&transitions);
	auto current = owned(obs_frontend_get_current_transition());
	s.transition = current ? text(obs_source_get_name(current.get())) : QString{};
	s.duration = obs_frontend_get_transition_duration();
	const float time = current ? obs_transition_get_time(current.get()) : 1.f;
	s.busy = (time > 0.f && time < 1.f) || !restoreTransition_.isEmpty() ||
		 (triggerGuard_.isValid() && triggerGuard_.elapsed() < 150);
	if (obs_get_module("downstream-keyer")) {
		const auto data = dskRequest("get_downstream_keyers", {{"view_name", ""}}, s.dskError);
		s.keyers = readDskKeyers(data, dskRequest, s.dskError);
		if (s.keyers.isEmpty() && s.dskError.isEmpty())
			s.dskError = "No hi ha canals DSK disponibles a la sortida principal.";
	} else
		s.dskError = "Downstream Keyer no està instal·lat o no s’ha carregat.";
	return s;
}
bool ObsBackend::execute(const Button &b, QString &error)
{
	const auto s = snapshot();
	if (!s.ready) {
		error = "Espera que s’acabi de carregar la col·lecció.";
		return false;
	}
	if (b.type == "scene") {
		if (s.busy) {
			error = "Hi ha una transició en curs.";
			return false;
		}
		auto scene = owned(b.target.isEmpty() ? obs_get_source_by_name(b.name.toUtf8().constData())
						      : obs_get_source_by_uuid(b.target.toUtf8().constData()));
		if (!scene || !obs_scene_from_source(scene.get())) {
			error = "Aquesta escena ja no existeix. Edita el botó per tornar-la a assignar.";
			return false;
		}
		if (s.studio)
			obs_frontend_set_current_preview_scene(scene.get());
		else
			obs_frontend_set_current_scene(scene.get());
		return true;
	}
	if (b.type != "dsk" && b.type != "dsk-clear") {
		error = "Tipus de botó no compatible.";
		return false;
	}
	if (!s.dskError.isEmpty()) {
		error = s.dskError;
		return false;
	}
	return executeDsk(b, s.keyers, dskRequest, error);
}
bool ObsBackend::transition(const QString &name, int ms, bool trigger, QString &error)
{
	const auto s = snapshot();
	if (!s.ready || s.busy) {
		error = "Espera: hi ha un canvi o una transició en curs.";
		return false;
	}
	if (trigger && !s.studio) {
		error = "AUTO i CUT requereixen el mode estudi.";
		return false;
	}
	auto next = findTransition(name);
	if (!next) {
		error = "La transició seleccionada no està disponible.";
		return false;
	}
	if (trigger && s.preview == s.program) {
		error = "Previ i programa ja mostren la mateixa escena.";
		return false;
	}
	if (name == "@cut") {
		restoreTransition_ = s.transition;
		restoreDuration_ = s.duration;
	}
	obs_frontend_set_current_transition(next.get());
	obs_frontend_set_transition_duration(std::clamp(ms, 50, 20000));
	if (trigger) {
		triggerGuard_.restart();
		obs_frontend_preview_program_trigger_transition();
	}
	return true;
}
} // namespace deck
