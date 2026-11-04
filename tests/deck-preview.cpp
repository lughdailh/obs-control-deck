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
#include <QBuffer>
#include <QDir>
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
	const bool website = app.arguments().contains("--website");
	if (website) {
		backend.state.transitions = {"Tall", "Fos", "Move"};
		backend.state.transitionKinds = {{"Tall", "cut_transition"}, {"Fos", "fade_transition"}, {"Move", "move_transition"}};
		backend.state.transition = "Fos";
		if (app.arguments().contains("--dsk-off")) backend.state.keyers[0].current.clear();
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
	if (website) {
		config.autoFit = true;
		config.presets = {{"Fos ràpid", "Fos", 100}, {"Fos suau", "Fos", 400}, {"Moviment", "Move", 600}, {"Tall", "Tall", 300}};
		for (int i = 0; i < config.buttons.size(); ++i) {
			config.buttons[i].image.clear(); config.buttons[i].color = "#a4a8ad";
			if (i < 6) config.hotkeys[config.buttons[i].id] = QJsonArray{deck::shortcutBinding(QKeySequence(QString("F%1").arg(i+1)))};
		}
		config.hotkeys["@auto"] = QJsonArray{deck::shortcutBinding(QKeySequence("Return"))};
		config.hotkeys[config.buttons[6].id] = QJsonArray{deck::shortcutBinding(QKeySequence("Ctrl+L"))};
		if (!app.arguments().contains("--plain")) {
			const auto artwork = app.arguments().indexOf("--artwork");
			if (artwork >= 0 && artwork + 1 < app.arguments().size()) {
				QImage image(app.arguments()[artwork+1]);
				QByteArray encoded; QBuffer buffer(&encoded); buffer.open(QIODevice::WriteOnly);
				image.scaled(512,512,Qt::KeepAspectRatio,Qt::SmoothTransformation).save(&buffer,"PNG");
				config.buttons[2].image = QString::fromLatin1(encoded.toBase64());
			}
			QImage bars(512,512,QImage::Format_RGB32); QPainter p(&bars);
			const QStringList colors = {"#d7d4c0", "#dcbf50", "#4eafb6", "#63aa6b", "#a06a9e", "#b35a62", "#4c688e"};
			for (int i=0; i<colors.size(); ++i) p.fillRect(i*74,0,74,512,QColor(colors[i])); p.end();
			QByteArray encoded; QBuffer buffer(&encoded); buffer.open(QIODevice::WriteOnly); bars.save(&buffer,"PNG");
			config.buttons[3].image = QString::fromLatin1(encoded.toBase64());
		}
	}
	widget.load(config);
	widget.resize(app.arguments().contains("--small") ? QSize(560, 480) : QSize(1040, 540));
	if (website) widget.resize(920,650);
	widget.show();
	qInfo() << "Tipografia de la botonera:" << QFontInfo(widget.font()).family();
	if (app.arguments().contains("--editor"))
		QTimer::singleShot(0, &widget, [&] {
			widget.findChild<QPushButton *>("lockDeck")->setChecked(false);
			auto *tiles = widget.findChild<QListWidget *>("sceneTiles");
			QMetaObject::invokeMethod(tiles, "itemDoubleClicked", Qt::DirectConnection,
				Q_ARG(QListWidgetItem *, tiles->item(0)));
		});
	const auto framesArg = app.arguments().indexOf("--pulse-frames");
	if (framesArg >= 0 && framesArg+1 < app.arguments().size()) {
		const QString directory = app.arguments()[framesArg+1]; QDir().mkpath(directory);
		auto *frames = new QTimer(&widget); frames->setInterval(40);
		QObject::connect(frames, &QTimer::timeout, &widget, [&, directory, frames, frame = 0]() mutable {
			auto *tiles = widget.findChild<QListWidget *>("sceneTiles");
			const auto region = tiles->visualItemRect(tiles->item(0)).united(tiles->visualItemRect(tiles->item(1)));
			tiles->viewport()->grab(region).save(directory+QString("/frame-%1.png").arg(frame,3,10,QChar('0')));
			if (++frame == 30) { frames->stop(); app.quit(); }
		}); frames->start();
	}
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
			QWidget *target = modal ? modal : &widget;
			const auto targetArg = app.arguments().indexOf("--capture-target");
			if (!modal && targetArg >= 0 && targetArg+1 < app.arguments().size())
				if (auto *found = widget.findChild<QWidget *>(app.arguments()[targetArg+1])) target = found;
			const auto buttonArg = app.arguments().indexOf("--capture-button");
			if (!modal && buttonArg >= 0 && buttonArg+1 < app.arguments().size()) {
				auto *tiles = widget.findChild<QListWidget *>("sceneTiles");
				tiles->viewport()->grab(tiles->visualItemRect(tiles->item(app.arguments()[buttonArg+1].toInt()))).save(app.arguments().last());
			} else target->grab().save(app.arguments().last());
			if (auto *dialog = qobject_cast<QDialog *>(modal)) dialog->reject();
			app.quit();
		});
	return app.exec();
}
