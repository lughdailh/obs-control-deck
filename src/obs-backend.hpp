#pragma once
#include "deck-model.hpp"
#include <QElapsedTimer>
#include <QPointer>
#include <obs-frontend-api.h>
#include <obs-hotkey.h>
#include <map>
#include <memory>

namespace deck {
class DeckWidget;
class ObsBackend : public Backend {
public:
	ObsBackend();
	~ObsBackend() override;
	void attach(DeckWidget *);
	Snapshot snapshot() override;
	bool execute(const Button &, QString &) override;
	bool transition(const QString &, int, bool, QString &) override;
	void save(const Config &) override;
	void replace(const Config &) override;
	Config portableConfig(const Config &) override;

private:
	Config config_;
	QPointer<DeckWidget> widget_;
	bool ready_ = true, shuttingDown_ = false;
	QString restoreTransition_;
	QByteArray rejectedConfig_;
	int restoreDuration_ = 300;
	QElapsedTimer triggerGuard_;
	struct Binding {
		ObsBackend *owner;
		QString id;
		obs_hotkey_id hotkey = OBS_INVALID_HOTKEY_ID;
	};
	std::map<QString, std::unique_ptr<Binding>> bindings_;
	obs_hotkey_id toggleHotkey_ = OBS_INVALID_HOTKEY_ID;
	void syncHotkeys();
	void saveToggleHotkey();
	static void buttonHotkey(void *, obs_hotkey_id, obs_hotkey_t *, bool);
	static void toggleHotkey(void *, obs_hotkey_id, obs_hotkey_t *, bool);
	static void eventCallback(enum obs_frontend_event, void *);
	static void saveCallback(obs_data_t *, bool, void *);
	void event(enum obs_frontend_event);
};
} // namespace deck
