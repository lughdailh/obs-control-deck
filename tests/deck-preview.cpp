#include "deck-widget.hpp"
#include "demo-backend.hpp"
#include "shortcut.hpp"
#include <QJsonArray>
#include <QApplication>
#include <QFontInfo>
#include <QDebug>
#include <QTimer>
#include <QDialog>
#include <QListWidget>
#include <QPushButton>
int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	if (app.arguments().contains("--host-theme"))
		app.setStyleSheet("QWidget { font-family: 'Helvetica'; }");
	DemoBackend backend;
	if (app.arguments().contains("--review-transitions")) {
		backend.state.transitions = {"Talla", "Fade", "Move", "Stinger"};
		backend.state.transitionKinds = {{"Talla", "cut_transition"}, {"Fade", "fade_transition"}, {"Move", "move_transition"}};
		backend.state.transition = "Fade";
	}
	deck::DeckWidget widget(backend);
	widget.setWindowTitle("Botonera · Demostració sense connexió a OBS");
	auto config = backend.demo();
	if (app.arguments().contains("--review-transitions"))
		config.presets = {{"ràpid", "Fade", 100}, {"tall", "Talla", 300}, {"moviment", "Move", 600}};
	config.autoFit = app.arguments().contains("--fit");
	config.locked = app.arguments().contains("--locked");
	if (app.arguments().contains("--control-images")) {
		config.controlImages.insert("@auto", config.buttons[0].image);
		config.controlImages.insert("@cut", config.buttons[1].image);
	}
	if (app.arguments().contains("--small")) config.tileSize = 80;
	if (app.arguments().contains("--plain")) for (auto &b : config.buttons) { b.image.clear(); b.color = "#a4a8ad"; }
	if (app.arguments().contains("--no-text")) config.showLabels = config.showStateText = false;
	if (app.arguments().contains("--badges")) {
		config.hotkeys[config.buttons[0].id] = QJsonArray{deck::shortcutBinding(QKeySequence("F1"))};
		config.hotkeys["@auto"] = QJsonArray{deck::shortcutBinding(QKeySequence("Space"))};
		config.hotkeys["@transition:" + backend.state.transitions.first()] = QJsonArray{deck::shortcutBinding(QKeySequence("F2"))};
	}
	widget.load(config);
	widget.resize(app.arguments().contains("--small") ? QSize(560, 480) : QSize(1040, 540));
	widget.show();
	qInfo() << "Tipografia de la botonera:" << QFontInfo(widget.font()).family();
	if (app.arguments().contains("--editor"))
		QTimer::singleShot(0, &widget, [&] {
			widget.findChild<QPushButton *>("lockDeck")->setChecked(false);
			auto *tiles = widget.findChild<QListWidget *>("sceneTiles");
			QMetaObject::invokeMethod(tiles, "itemDoubleClicked", Qt::DirectConnection,
				Q_ARG(QListWidgetItem *, tiles->item(0)));
		});
	if (app.arguments().contains("--capture"))
		QTimer::singleShot(500, &widget, [&] {
			if (app.arguments().contains("--pulse-low") || app.arguments().contains("--pulse-high")) {
				auto *tiles = widget.findChild<QListWidget *>("sceneTiles");
				const double light = app.arguments().contains("--pulse-low") ? .08 : 1.0;
				tiles->setProperty("previewLight", light); tiles->viewport()->setProperty("previewLight", light);
			}
            if (app.arguments().contains("--geometry")) {
                for (auto *preset : widget.findChildren<QPushButton *>("transitionPreset"))
                    qInfo() << preset->text() << preset->geometry() << preset->minimumSize() << preset->maximumSize();
            }
			auto *modal = QApplication::activeModalWidget();
			(modal ? modal : &widget)->grab().save(app.arguments().last());
			if (auto *dialog = qobject_cast<QDialog *>(modal)) dialog->reject();
			app.quit();
		});
	return app.exec();
}
