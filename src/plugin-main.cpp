#include "deck-widget.hpp"
#include "obs-backend.hpp"
#include <obs-module.h>
#include <QPointer>
#include <memory>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-control-deck", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Configurable scene and DSK switcher dock with transition presets.";
}
static std::unique_ptr<deck::ObsBackend> backend;
static QPointer<deck::DeckWidget> dock;
bool obs_module_load(void)
{
	return true;
}
void obs_module_post_load(void)
{
	backend = std::make_unique<deck::ObsBackend>();
	dock = new deck::DeckWidget(*backend, static_cast<QWidget *>(obs_frontend_get_main_window()));
	backend->attach(dock);
	if (!obs_frontend_add_dock_by_id("obs-control-deck", "Botonera", dock)) {
		delete dock;
		backend.reset();
		blog(LOG_ERROR, "[obs-control-deck] Cannot register dock.");
		return;
	}
	blog(LOG_INFO, "[obs-control-deck] Loaded 0.2.9");
}
void obs_module_unload(void)
{
	obs_frontend_remove_dock("obs-control-deck");
	if (dock)
		delete dock;
	backend.reset();
}
