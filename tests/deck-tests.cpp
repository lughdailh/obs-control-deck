#include "deck-widget.hpp"
#include "shortcut.hpp"
#include "dsk-control.hpp"
#include <QKeySequenceEdit>
#include <QFontDatabase>
#include <QFontInfo>
#include <QLabel>
#include "demo-backend.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QApplication>
#include <QDialog>
#include <QCheckBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QSplitterHandle>
#include <QSpinBox>
#include <QMenu>
#include <QTimer>
#include <QDialogButtonBox>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QTemporaryDir>
#include <QUrl>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QAbstractItemDelegate>
#include <QEventLoop>
#include <QStyleOptionViewItem>
#include <QKeyEvent>
#include <stdexcept>
#include <iostream>
#define QVERIFY(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)
#define QCOMPARE(a, b) QVERIFY((a)==(b))
namespace QTest {
void qWait(int)
{
	QApplication::processEvents();
}
void mouseClick(QWidget *w, Qt::MouseButton b, Qt::KeyboardModifiers mods, QPoint point)
{
	QMouseEvent press(QEvent::MouseButtonPress, QPointF(point), QPointF(w->mapToGlobal(point)), b, b, mods);
	QApplication::sendEvent(w, &press);
	QMouseEvent release(QEvent::MouseButtonRelease, QPointF(point), QPointF(w->mapToGlobal(point)), b, Qt::NoButton,
			    mods);
	QApplication::sendEvent(w, &release);
	QApplication::processEvents();
}
} // namespace QTest

class DeckTests {
public:
	void typographyOverridesHostTheme()
	{
		const auto originalStyle = qApp->styleSheet();
		qApp->setStyleSheet("QWidget { font-family: 'Helvetica'; }");
		QLabel outside("Host OBS"); outside.ensurePolished();
		const auto hostFamily = QFontInfo(outside.font()).family();
		DemoBackend b;
		deck::DeckWidget w(b); w.load(b.demo()); w.show();
		QDialog dialog(&w); QLabel label("Editor", &dialog); dialog.show();
		QApplication::processEvents();
		if (QFontDatabase::families().contains("Montserrat", Qt::CaseInsensitive)) {
			QCOMPARE(QFontInfo(w.font()).family(), QString("Montserrat"));
			for (auto *child : w.findChildren<QWidget *>()) {
				child->ensurePolished();
				QCOMPARE(QFontInfo(child->font()).family(), QString("Montserrat"));
			}
		}
		QCOMPARE(QFontInfo(outside.font()).family(), hostFamily);
		qApp->setStyleSheet(originalStyle);
	}
	void transitionColumnsLayout()
	{
		DemoBackend b;
		b.state.transitions = {"Couper", "Fondu", "Moviment", "Stinger"};
		b.state.transitionKinds = {{"Couper", "cut_transition"}, {"Fondu", "fade_transition"}};
		b.state.transition = "Fondu";
		auto config = b.demo(); config.locked = true; config.autoFit = true;
		config.presets = {{"fast", "Fondu", 100}, {"instantani", "Couper", 300}, {"suau", "Moviment", 300}};
		deck::DeckWidget w(b); w.load(config); w.show(); QApplication::processEvents();
		QEventLoop settling; QTimer::singleShot(60, &settling, &QEventLoop::quit); settling.exec();
		auto *columns = w.findChild<QSplitter *>("deckColumns");
		auto *center = w.findChild<QWidget *>("transitionColumn");
		auto *autoButton = w.findChild<QPushButton *>("autoTransition");
		auto *choices = w.findChild<QWidget *>("transitionChoices");
		auto *duration = w.findChild<QWidget *>("durationChoices");
		QVERIFY(columns); QCOMPARE(columns->count(), 2);
		QVERIFY(!columns->handle(1)->isEnabled());
		for (auto size : {QSize(1200,560), QSize(700,420), QSize(520,320), QSize(1400,780)}) {
			w.resize(size); QApplication::processEvents(); QApplication::processEvents();
			QVERIFY(center->rect().contains(autoButton->geometry()));
			QVERIFY(autoButton->width() <= center->width());
			QCOMPARE(autoButton->height(), 64);
			auto *presetLabel = w.findChild<QLabel *>("transitionPresetLabel");
			QVERIFY(presetLabel->mapToGlobal(QPoint()).y() > autoButton->mapToGlobal(autoButton->rect().bottomLeft()).y());
			const int durationBottom = duration->mapToGlobal(duration->rect().bottomLeft()).y();
			const int actionTop = autoButton->mapToGlobal(QPoint()).y();
			QVERIFY(actionTop > durationBottom);
			QVERIFY(actionTop - durationBottom <= 8);
			const QStringList codes = {"C", "F", "M", "S"};
			int codeIndex = 0;
			for (auto *choice : choices->findChildren<QPushButton *>("transitionChoice")) {
				QCOMPARE(choice->text(), codes[codeIndex++]);
				QVERIFY(choices->rect().contains(choice->geometry()));
				QVERIFY(choice->mapToGlobal(choice->rect().bottomLeft()).y() < duration->mapToGlobal(QPoint()).y());
			}
			QVERIFY(center->mapToGlobal(QPoint()).x() >= columns->widget(0)->mapToGlobal(columns->widget(0)->rect().topRight()).x());
		}
		auto presets = w.findChildren<QPushButton *>("transitionPreset");
		QCOMPARE(presets.size(), 3); QCOMPARE(presets[0]->text(), QString("F100"));
		for (int i = 0; i < presets.size(); ++i) {
			QVERIFY(presets[i]->height() >= 36);
			for (int j = i + 1; j < presets.size(); ++j) QVERIFY(!presets[i]->geometry().intersects(presets[j]->geometry()));
		}
		QCOMPARE(presets[1]->text(), QString("C")); QCOMPARE(presets[2]->text(), QString("M300"));
		QVERIFY(presets[0]->toolTip().contains("fast"));
		presets[0]->click(); QCOMPARE(b.state.duration, 100); QCOMPARE(b.triggers, 0);
		QPushButton *time400 = nullptr;
		for (auto *choice : duration->findChildren<QPushButton *>("durationChoice"))
			if (choice->property("milliseconds").toInt() == 400) time400 = choice;
		QVERIFY(time400); time400->click(); QCOMPARE(b.state.duration, 400); QCOMPARE(b.triggers, 0);
		w.activateButton("@transition:Couper");
		for (auto *choice : duration->findChildren<QPushButton *>("durationChoice")) {
			QVERIFY(!choice->isEnabled()); QVERIFY(!choice->isChecked());
		}
		config = w.config(); config.locked = false; w.load(config); QApplication::processEvents();
		QVERIFY(columns->handle(1)->isEnabled());
		columns->setSizes({700,230});
		QMetaObject::invokeMethod(columns, "splitterMoved", Qt::DirectConnection, Q_ARG(int, 700), Q_ARG(int, 1));

		QCOMPARE(b.saved.transitionSettingsWidth, columns->sizes()[1]);
		deck::Config restored; QString error;
		QVERIFY(deck::Config::parse(b.saved.json(), restored, error));
		QCOMPARE(restored.transitionColumnWidth, b.saved.transitionColumnWidth);
		QCOMPARE(restored.transitionSettingsWidth, b.saved.transitionSettingsWidth);
		w.load(restored); QApplication::processEvents();
		QCOMPARE(columns->sizes()[1], restored.transitionSettingsWidth);
		auto invalid = restored.json(); invalid["transitionColumnWidth"] = -1;
		QVERIFY(!deck::Config::parse(invalid, restored, error));
		invalid = b.saved.json(); invalid.remove("transitionColumnWidth"); invalid.remove("transitionSettingsWidth");
		QVERIFY(deck::Config::parse(invalid, restored, error));
		QCOMPARE(restored.transitionColumnWidth, 180); QCOMPARE(restored.transitionSettingsWidth, 180);
	}
	void transitionActionStaysVisibleAcrossFocusAndBusyChanges()
	{
		DemoBackend backend;
		backend.state.transitions = {"Tall", "Fos", "Move", "Luma", "Stinger", "Swipe", "Slide", "Fade to color", "Wipe"};
		backend.state.transitionKinds = {{"Tall", "cut_transition"}, {"Fos", "fade_transition"}};
		backend.state.transition = "Fos";
		auto config = backend.demo(); config.locked = true;
		for (int i = 0; i < 20; ++i) config.presets.append({QString::number(i), "Fos", 100 + i * 50});
		deck::DeckWidget widget(backend); widget.load(config); widget.resize(600, 340); widget.show();
		QApplication::processEvents(); QApplication::processEvents();
		auto *action = widget.findChild<QPushButton *>("autoTransition");
		auto *selectors = widget.findChild<QWidget *>("transitionSelectors");
		auto *presets = widget.findChild<QScrollArea *>("transitionPresetScroll");
		QVERIFY(selectors); QVERIFY(presets);
		const QPoint fixed = action->mapTo(&widget, QPoint());
		QVERIFY(widget.rect().contains(QRect(fixed, action->size())));
		auto fullyVisible = [&](QWidget *control) {
			for (auto *parent = control->parentWidget(); parent; parent = parent->parentWidget()) {
				QVERIFY(parent->rect().contains(QRect(control->mapTo(parent, QPoint()), control->size())));
				if (parent == &widget) break;
			}
		};
		fullyVisible(action);
		QVERIFY(widget.rect().contains(QRect(selectors->mapTo(&widget, QPoint()), selectors->size())));
		QVERIFY(!widget.findChild<QScrollArea *>("transitionSelectorScroll"));
		for (auto *choice : widget.findChildren<QPushButton *>("transitionChoice")) {
			fullyVisible(choice);
			const auto area = choice->parentWidget()->rect();
			QVERIFY(area.contains(choice->geometry()));
			const QRect visible(choice->mapTo(selectors, QPoint()), choice->size());
			QVERIFY(selectors->rect().contains(visible));
			QVERIFY(widget.rect().contains(QRect(choice->mapTo(&widget, QPoint()), choice->size())));
		}
		auto presetButtons = widget.findChildren<QPushButton *>("transitionPreset");
		for (int i = 0; i < 6; ++i) {
			presets->ensureWidgetVisible(presetButtons.last());
			presetButtons.last()->setFocus();
			backend.state.busy = true; widget.refresh(); QApplication::processEvents();
			backend.state.busy = false; widget.refresh(); QApplication::processEvents();
			QCOMPARE(action->mapTo(&widget, QPoint()), fixed);
			QVERIFY(widget.rect().contains(QRect(fixed, action->size())));
			QTest::mouseClick(action, Qt::LeftButton, Qt::NoModifier, action->rect().center());
			QCOMPARE(action->mapTo(&widget, QPoint()), fixed);
		}
		QCOMPARE(backend.triggers, 6);
		for (auto *choice : widget.findChildren<QPushButton *>("durationChoice")) {
			if (!choice->isVisible()) continue;
			fullyVisible(choice);
			QVERIFY(widget.rect().contains(QRect(choice->mapTo(&widget, QPoint()), choice->size())));
			QVERIFY(choice->mapTo(&widget, choice->rect().bottomLeft()).y() < action->mapTo(&widget, QPoint()).y());
		}
		config.presets.clear(); widget.load(config); QApplication::processEvents(); QApplication::processEvents();
		QVERIFY(!widget.findChild<QLabel *>("transitionPresetLabel")->isVisible());
		QVERIFY(!presets->isVisible());
		QCOMPARE(action->height(), 64);
	}
	void editingShortcutBadges()
	{
		DemoBackend b; deck::DeckWidget w(b); auto config = b.demo();
		config.locked = false;
		config.hotkeys[config.buttons[0].id] = QJsonArray{deck::shortcutBinding(QKeySequence("F1"))};
		config.hotkeys["@auto"] = QJsonArray{deck::shortcutBinding(QKeySequence("Space"))};
		w.load(config); w.show(); QApplication::processEvents();
		auto *tiles = w.findChild<QListWidget *>("sceneTiles");
		auto *autoButton = w.findChild<QPushButton *>("autoTransition");
		QCOMPARE(tiles->item(0)->data(Qt::UserRole + 4).toString(), QString("F1"));
		QVERIFY(!autoButton->property("shortcutLabel").toString().isEmpty());
		QVERIFY(tiles->item(1)->data(Qt::UserRole + 4).toString().isEmpty());
		config.locked = true; w.load(config); QApplication::processEvents();
		QVERIFY(tiles->item(0)->data(Qt::UserRole + 4).toString().isEmpty());
		QVERIFY(autoButton->property("shortcutLabel").toString().isEmpty());
		QCOMPARE(w.config().hotkeys, config.hotkeys);
	}
	void compactTransitionAndDskToggle()
	{
		DemoBackend b;
		deck::DeckWidget w(b);
		auto config = b.demo(); config.locked = true;
		w.load(config); w.resize(1040, 540); w.show(); QApplication::processEvents();
		auto *autoButton = w.findChild<QPushButton *>("autoTransition");
		auto *cut = w.findChild<QPushButton *>("cutTransition");
		QCOMPARE(autoButton->text(), QString("TRANSICIÓ"));
		QVERIFY(autoButton->isVisible()); QVERIFY(!cut->isVisible());
		QVERIFY(!w.findChild<QLabel *>("transitionHeading"));
		for (auto *choice : w.findChildren<QPushButton *>("transitionChoice")) {
			QVERIFY(choice->width() >= 28); QCOMPARE(choice->height(), 28);
			QVERIFY(!choice->accessibleName().isEmpty());
		}
		const auto dsk = config.buttons[6];
		QCOMPARE(dsk.type, QString("dsk"));
		auto *tiles = w.findChild<QListWidget *>("sceneTiles");
		w.activateButton(dsk.id); // Active logo disappears; scene monitors stay put.
		QVERIFY(b.state.keyers[0].current.isEmpty());
		QVERIFY(tiles->item(6)->toolTip().contains("Clic per mostrar"));
		w.activateButton(dsk.id);
		QCOMPARE(b.state.keyers[0].current, dsk.name);
		QVERIFY(tiles->item(6)->toolTip().contains("Clic per amagar"));
		w.activateButton(config.buttons[7].id); // Another composition on the same channel.
		QCOMPARE(b.state.keyers[0].current, config.buttons[7].name);
		QCOMPARE(b.state.program, QString("a")); QCOMPARE(b.state.preview, QString("b"));
		QCOMPARE(b.triggers, 0);
	}
	void illuminatedNumbersAndVisibility()
	{
		DemoBackend b;
		deck::DeckWidget w(b);
		auto config = b.demo(); config.showLabels = config.showStateText = false;
		config.columns = 20; config.rows = 10;
		config.normalize(); w.load(config);
		deck::Config parsed; QString error;
		QVERIFY(deck::Config::parse(config.json(), parsed, error));
		QVERIFY(!parsed.showLabels && !parsed.showStateText);
		auto legacy = config.json(); legacy.remove("showLabels"); legacy.remove("showStateText");
		QVERIFY(deck::Config::parse(legacy, parsed, error)); QVERIFY(parsed.showLabels && parsed.showStateText);
		auto invalid = config.json(); invalid["showLabels"] = "yes";
		const auto before = parsed.json();
		QVERIFY(!deck::Config::parse(invalid, parsed, error)); QCOMPARE(parsed.json(), before);
		auto *tiles = w.findChild<QListWidget *>("sceneTiles");
		auto render = [&](int row, int side, const QString &state, double light) {
			tiles->item(row)->setData(Qt::UserRole + 1, state);
			tiles->viewport()->setProperty("previewLight", light);
			QImage result(side, side, QImage::Format_ARGB32_Premultiplied); result.fill(Qt::black);
			QPainter painter(&result); QStyleOptionViewItem option;
			option.rect = QRect(0, 0, side, side); option.font = w.font(); option.palette = w.palette();
			option.widget = tiles->viewport();
			tiles->itemDelegate()->paint(&painter, option, tiles->model()->index(row, 0));
			painter.end(); return result;
		};
		// Numbers remain visible on artwork and on a three-digit empty position.
		for (int row : {0, 199}) for (int side : {48, 80, 112, 200}) {
			auto image = render(row, side, "", 1);
			int dark = 0;
			for (int y=side/3; y<side*2/3; ++y) for (int x=side/4; x<side*3/4; ++x) {
				const auto color = image.pixelColor(x,y);
				if (color.red() < 115 && color.green() < 115 && color.blue() < 115) ++dark;
			}
			QVERIFY(dark > 5);
		}
		const auto dim = render(1, 80, "PREVI", .08);
		const auto lit = render(1, 80, "PREVI", 1);
		QVERIFY(lit.pixelColor(40,16).lightness() > dim.pixelColor(40,16).lightness() + 20);
		QCOMPARE(render(0, 80, "EN DIRECTE", .08), render(0, 80, "EN DIRECTE", 1));
		// Both options preview live and cancel restores the original settings.
		config = b.demo(); w.load(config); w.show(); QApplication::processEvents();
		QTimer::singleShot(0, [&] {
			auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget()); QVERIFY(menu);
			for (auto *action : menu->actions()) if (action->text() == "Graella i mida…") {
				QTimer::singleShot(0, [&] {
					auto *dialog = QApplication::activeModalWidget(); QVERIFY(dialog);
					dialog->findChild<QCheckBox *>("showLabels")->setChecked(false);
					dialog->findChild<QCheckBox *>("showStateText")->setChecked(false);
					QVERIFY(!w.config().showLabels && !w.config().showStateText);
					QVERIFY(tiles->viewport()->property("hideLabels").toBool());
					qobject_cast<QDialog *>(dialog)->reject();
				}); action->trigger(); menu->close(); return;
			}
		});
		QMetaObject::invokeMethod(tiles, "customContextMenuRequested", Qt::DirectConnection, Q_ARG(QPoint, QPoint(10,10)));
		QVERIFY(w.config().showLabels && w.config().showStateText);
		QTimer::singleShot(0, [&] {
			auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget()); QVERIFY(menu);
			for (auto *action : menu->actions()) if (action->text() == "Graella i mida…") {
				QTimer::singleShot(0, [&] {
					auto *dialog = QApplication::activeModalWidget(); QVERIFY(dialog);
					dialog->findChild<QCheckBox *>("showLabels")->setChecked(false);
					dialog->findChild<QCheckBox *>("showStateText")->setChecked(false);
					dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
				}); action->trigger(); menu->close(); return;
			}
		});
		QMetaObject::invokeMethod(tiles, "customContextMenuRequested", Qt::DirectConnection, Q_ARG(QPoint, QPoint(10,10)));
		QVERIFY(!b.saved.showLabels && !b.saved.showStateText);
		// Observe a real animation cycle, independent from the snapshot polling timer.
		QEventLoop loop; QTimer sample; sample.setInterval(60);
		double minimum = 1, maximum = 0;
		QObject::connect(&sample, &QTimer::timeout, [&] {
			const auto level = tiles->viewport()->property("previewLight").toDouble();
			minimum = std::min(minimum, level); maximum = std::max(maximum, level);
		});
		sample.start(); QTimer::singleShot(1300, &loop, &QEventLoop::quit); loop.exec();
		QVERIFY(maximum - minimum > .7);
	}
	void dskSelectionDiffersFromFocus()
	{
		QString selected, focus, error;
		QJsonObject metadata;
		auto updateMetadata = [&] {
			QJsonObject channel{{"name", "DSK 1"}, {"scene", focus}, {"tie", false}};
			channel["scenes"] = QJsonArray{QJsonObject{{"name", "Gràfic"}}, QJsonObject{{"name", "Rètol"}}};
			metadata = QJsonObject{{"downstream_keyers", QJsonArray{channel}}};
		};
		int selections = 0;
		deck::DskRequest api = [&](const QString &type, const QJsonObject &data, QString &) {
			QCOMPARE(data["dsk_name"].toString(), QString("DSK 1"));
			QCOMPARE(data["view_name"].toString(), QString{});
			if (type == "dsk_get_scene") return QJsonObject{{"success", true}, {"scene", selected}};
			QCOMPARE(type, QString("dsk_select_scene"));
			selected = data["scene"].toString(); ++selections;
			return QJsonObject{{"success", true}};
		};
		deck::Button graphic; graphic.type = "dsk"; graphic.channel = "DSK 1"; graphic.name = "Gràfic";
		// API selection can change without changing the QListWidget focus item.
		for (const auto &staleFocus : {QString{}, QString("Gràfic"), QString("Rètol")}) {
			focus = staleFocus; selected.clear(); updateMetadata();
			for (int click = 0; click < 6; ++click) {
				const auto keyers = deck::readDskKeyers(metadata, api, error);
				QVERIFY(error.isEmpty()); QCOMPARE(keyers[0].current, selected);
				QVERIFY(deck::executeDsk(graphic, keyers, api, error));
				QCOMPARE(selected, click % 2 == 0 ? QString("Gràfic") : QString{});
			}
		}
		QCOMPARE(selections, 18);
		// External DSK selection and explicit clear also use the current selection.
		selected = "Rètol";
		QVERIFY(deck::executeDsk(graphic, deck::readDskKeyers(metadata, api, error), api, error));
		QCOMPARE(selected, QString("Gràfic"));
		graphic.type = "dsk-clear";
		QVERIFY(deck::executeDsk(graphic, deck::readDskKeyers(metadata, api, error), api, error));
		QVERIFY(selected.isEmpty());
		deck::DskRequest broken = [](const QString &, const QJsonObject &, QString &) { return QJsonObject{{"success", false}}; };
		QVERIFY(deck::readDskKeyers(metadata, broken, error).isEmpty()); QVERIFY(!error.isEmpty());
	}
	void portableRoundtrip()
	{
		DemoBackend b;
		const auto input = b.demo();
		deck::Config output;
		QString error;
		QVERIFY(deck::Config::parse(QJsonDocument::fromJson(QJsonDocument(input.json()).toJson()).object(),
					    output, error));
		QCOMPARE(output.buttons[0].image, input.buttons[0].image);
		QCOMPARE(output.buttons[7].channel, QString("DSK 1"));
		QCOMPARE(output.presets[1].duration, 1000);
	}
	void invalidImportIsAtomic()
	{
		DemoBackend b;
		deck::Config unchanged = b.demo();
		const auto expected = unchanged.json();
		QString error;
		auto bad = expected;
		bad["schema"] = 2;
		QVERIFY(!deck::Config::parse(bad, unchanged, error));
		QCOMPARE(unchanged.json(), expected);
		bad = expected;
		auto buttons = bad["buttons"].toArray();
		auto button = buttons[0].toObject();
		button["image"] = "invalid image";
		buttons[0] = button;
		bad["buttons"] = buttons;
		QVERIFY(!deck::Config::parse(bad, unchanged, error));
		QCOMPARE(unchanged.json(), expected);
		bad = expected;
		buttons = bad["buttons"].toArray();
		buttons.append(buttons[0]);
		bad["buttons"] = buttons;
		QVERIFY(!deck::Config::parse(bad, unchanged, error));
		QCOMPARE(unchanged.json(), expected);
	}
	void studioAndPresetInteraction()
	{
		DemoBackend b;
		deck::DeckWidget w(b);
		w.load(b.demo());
		w.findChild<QPushButton *>("lockDeck")->click();
		w.resize(1000, 480);
		w.show();
		QTest::qWait(20);
		auto *tiles = w.findChild<QListWidget *>("sceneTiles");
		QVERIFY(tiles);
		QTest::mouseClick(tiles->viewport(), Qt::LeftButton, Qt::NoModifier,
				  tiles->visualItemRect(tiles->item(2)).center());
		QCOMPARE(b.state.preview, QString("c"));
		QCOMPARE(b.state.program, QString("a"));
		QPushButton *preset = nullptr;
		for (auto *p : w.findChildren<QPushButton *>())
			if (p->property("controlId").toString() == "@preset:" + w.config().presets[1].id)
				preset = p;
		QVERIFY(preset);
		QVERIFY(w.config().locked);
		QVERIFY(preset->isVisible());
		QVERIFY(preset->isEnabled());
		preset->click();
		QCOMPARE(b.state.duration, 1000);
		QCOMPARE(b.triggers, 0);
		w.findChild<QPushButton *>("autoTransition")->click();
		QCOMPARE(b.state.program, QString("c"));
		QCOMPARE(b.triggers, 1);
	}
	void editAndLifecycleInterlocks()
	{
		DemoBackend b;
		deck::DeckWidget w(b);
		w.load(b.demo());
		w.findChild<QPushButton *>("lockDeck")->click();
		w.resize(1000, 480);
		w.show();
		QTest::qWait(20);
		QDialog settings(&w);
		settings.setModal(true);
		settings.show();
		QApplication::processEvents();
		w.activateButton("b");
		QCOMPARE(b.executions, 0);
		settings.accept();
		QApplication::processEvents();

		auto *edit = w.findChild<QPushButton *>("editMode");
		auto *tiles = w.findChild<QListWidget *>("sceneTiles");
		edit->click();
		QTest::mouseClick(tiles->viewport(), Qt::LeftButton, Qt::NoModifier,
				  tiles->visualItemRect(tiles->item(1)).center());
		QCOMPARE(b.executions, 0);
		w.activateButton("b");
		QCOMPARE(b.executions, 0);
		QVERIFY(!w.findChild<QPushButton *>("autoTransition")->property("operational").toBool());
		edit->click();
		b.state.ready = false;
		w.refresh();
		QTest::mouseClick(tiles->viewport(), Qt::LeftButton, Qt::NoModifier,
				  tiles->visualItemRect(tiles->item(2)).center());
		QCOMPARE(b.executions, 0);
		QVERIFY(!w.findChild<QPushButton *>("addButton")->isEnabled());
		b.state.ready = true;
		b.state.studio = false;
		w.refresh();
		QVERIFY(!w.findChild<QPushButton *>("autoTransition")->property("operational").toBool());
		b.state.studio = true;
		b.state.busy = true;
		w.refresh();
		QVERIFY(!w.findChild<QPushButton *>("cutTransition")->property("operational").toBool());
	}
	void incrementalLayout()
	{
		DemoBackend b;
		deck::DeckWidget w(b);
		w.resize(940, 500);
		w.show();
		auto c = b.demo();
		c.buttons.resize(1);
		w.load(c);
		QApplication::processEvents();
		c = b.demo();
		c.buttons.resize(2);
		w.load(c);
		QApplication::processEvents();
		auto *tiles = w.findChild<QListWidget *>("sceneTiles");
		QVERIFY(!tiles->visualItemRect(tiles->item(0)).intersects(tiles->visualItemRect(tiles->item(1))));
		QVERIFY(tiles->viewport()->rect().contains(tiles->visualItemRect(tiles->item(1))));
	}
	void dropImageEmbedsWithoutExecuting()
	{
		DemoBackend b;
		deck::DeckWidget w(b);
		w.load(b.demo());
		w.resize(940, 500);
		w.show();
		QApplication::processEvents();
		QTemporaryDir dir;
		const QString path = dir.filePath("portrait.png");
		QImage source(300, 900, QImage::Format_RGB32);
		source.fill(Qt::blue);
		QVERIFY(source.save(path));
		auto *tiles = w.findChild<QListWidget *>("sceneTiles");
		auto *viewport = tiles->viewport();
		const auto pos = tiles->visualItemRect(tiles->item(0)).center();
		QMimeData mime;
		mime.setUrls({QUrl::fromLocalFile(path)});
		QDragEnterEvent enter(pos, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
		QApplication::sendEvent(viewport, &enter);
		QVERIFY(enter.isAccepted());
		QDragMoveEvent move(pos, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
		QApplication::sendEvent(viewport, &move);
		QVERIFY(move.isAccepted());
		QDropEvent drop(QPointF(pos), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
		QApplication::sendEvent(viewport, &drop);
		QVERIFY(drop.isAccepted());
		QCOMPARE(b.executions, 0);
		QVERIFY(!w.config().buttons[0].image.isEmpty());
		QImage embedded;
		QVERIFY(embedded.loadFromData(QByteArray::fromBase64(w.config().buttons[0].image.toLatin1()), "PNG"));
		QCOMPARE(embedded.height(), 512);
		QVERIFY(embedded.width() < embedded.height());
		QCOMPARE(b.saved.buttons[0].image, w.config().buttons[0].image);
		QFile::remove(path);
		QVERIFY(embedded.loadFromData(QByteArray::fromBase64(b.saved.buttons[0].image.toLatin1()), "PNG"));
	}
	void slotsDragAssignmentAndLock()
	{
		DemoBackend b;
		deck::DeckWidget w(b);
		w.load(b.demo());
		w.resize(1000, 600);
		w.show();
		QApplication::processEvents();
		QCOMPARE(w.config().buttons.size(), 12);
		QWidget keyer;
		keyer.setObjectName("DSK 1");
		QListWidget source(&keyer);
		source.setObjectName("scenes");
		source.setDragDropMode(QAbstractItemView::InternalMove);
		source.addItem("Rètol convidat");
		source.setCurrentRow(0);
		w.assignFromSource(&source, 10);
		QCOMPARE(w.config().buttons[10].type, QString("dsk"));
		QCOMPARE(w.config().buttons[10].channel, QString("DSK 1"));
		QCOMPARE(source.count(), 1);
		QCOMPARE(b.executions, 0);
		QListWidget scenes;
		scenes.setObjectName("scenes");
		scenes.addItem("Pla general");
		scenes.setCurrentRow(0);
		w.assignFromSource(&scenes, 11);
		QCOMPARE(w.config().buttons[11].target, QString("c"));
		auto *edit = w.findChild<QPushButton *>("editMode");
		w.moveButton("a", 3);
		QCOMPARE(w.config().buttons[3].id, QString("a"));
		QCOMPARE(w.config().buttons.size(), 12);
		auto *lock = w.findChild<QPushButton *>("lockDeck");
		lock->click();
		QVERIFY(w.config().locked);
		QVERIFY(!edit->isChecked());
		const auto before = w.config().json();
		w.moveButton("a", 0);
		w.assignFromSource(&source, 0);
		QCOMPARE(w.config().json(), before);
		auto *tiles = w.findChild<QListWidget *>("sceneTiles");
		QMimeData mime;
		QImage img(50, 50, QImage::Format_RGB32);
		img.fill(Qt::green);
		mime.setImageData(img);
		QDragEnterEvent enter(QPoint(50, 50), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
		QApplication::sendEvent(tiles->viewport(), &enter);
		QVERIFY(!enter.isAccepted());
		QCOMPARE(w.config().json(), before);
		deck::Config restored;
		QString error;
		QVERIFY(deck::Config::parse(before, restored, error));
		QVERIFY(restored.locked);
		QCOMPARE(restored.slotCount, 12);
		w.activateButton("a");
		QCOMPARE(b.executions, 1);
	}
	void gridAndContextualConfiguration()
	{
		DemoBackend b;
		deck::DeckWidget w(b);
		w.load(b.demo()); w.resize(1000, 600); w.show(); QApplication::processEvents();
		auto *tiles = w.findChild<QListWidget *>("sceneTiles");
		QCOMPARE(tiles->visualItemRect(tiles->item(3)).y(), tiles->visualItemRect(tiles->item(0)).y());
		QVERIFY(tiles->visualItemRect(tiles->item(4)).y() > tiles->visualItemRect(tiles->item(0)).y());
		w.resize(500, 600); QApplication::processEvents();
		QCOMPARE(tiles->visualItemRect(tiles->item(3)).y(), tiles->visualItemRect(tiles->item(0)).y());
		// Use the real context menu and settings dialog, including occupied-slot protection.
		QTimer::singleShot(0, [&] {
			auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
			QVERIFY(menu);
			for (auto *action : menu->actions()) if (action->text() == "Graella i mida…") {
				QTimer::singleShot(0, [&] {
					auto *dialog = w.findChild<QDialog *>("gridSettings"); QVERIFY(dialog);
					dialog->findChild<QSpinBox *>("gridColumns")->setValue(3);
					dialog->findChild<QSpinBox *>("gridRows")->setValue(4);
					dialog->findChild<QCheckBox *>("autoFit")->setChecked(true);
					QApplication::processEvents();
					QCOMPARE(w.config().columns, 3); QCOMPARE(w.config().rows, 4);
					QVERIFY(w.config().autoFit);
					QVERIFY(b.saved.buttons.isEmpty());
					if (!qEnvironmentVariableIsEmpty("DECK_CAPTURE_DIR"))
						dialog->grab().save(qEnvironmentVariable("DECK_CAPTURE_DIR") + "/graella-adaptacio.png");
					QVERIFY(!dialog->findChild<QSpinBox *>("tilePixels")->isEnabled());
					dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
				});
				action->trigger(); menu->close(); break;
			}
		});
		QMetaObject::invokeMethod(tiles, "customContextMenuRequested", Qt::DirectConnection, Q_ARG(QPoint, QPoint(10, 10)));
		QCOMPARE(w.config().columns, 3); QCOMPARE(w.config().rows, 4);
		QVERIFY(tiles->visualItemRect(tiles->item(3)).y() > tiles->visualItemRect(tiles->item(0)).y());
		deck::Config restored; QString error;
		QVERIFY(deck::Config::parse(w.config().json(), restored, error));
		QCOMPARE(restored.columns, 3); QCOMPARE(restored.rows, 4);
		QVERIFY(restored.autoFit);
		auto legacy = restored.json(); legacy.remove("columns"); legacy.remove("rows");
		QVERIFY(deck::Config::parse(legacy, restored, error)); QCOMPARE(restored.columns, 4);
		w.moveButton("a", 1); QCOMPARE(w.config().buttons[1].id, QString("a"));
		QCOMPARE(b.executions, 0);
		w.findChild<QPushButton *>("lockDeck")->click();
		for (auto *button : w.findChildren<QPushButton *>())
			if (button->objectName() == "addButton" || button->objectName() == "lockDeck" ||
			    button->objectName() == "editMode") QVERIFY(!button->isVisible());
		QTimer::singleShot(0, [&] {
			auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget()); QVERIFY(menu);
			QCOMPARE(menu->actions().size(), 1);
			menu->actions().first()->trigger(); menu->close();
		});
		QMetaObject::invokeMethod(tiles, "customContextMenuRequested", Qt::DirectConnection, Q_ARG(QPoint, QPoint(10, 10)));
		// The configure action and the persisted mode must change together.
		QVERIFY(!w.config().locked);
		QVERIFY(w.findChild<QPushButton *>("editMode")->isChecked());
	}
	void shortcutEditorAndLivePreviewCancellation()
	{
		for (const QKeySequence &sequence : {QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_A),
		     QKeySequence(Qt::SHIFT | Qt::Key_F12), QKeySequence(Qt::Key_Space),
		     QKeySequence(Qt::META | Qt::Key_Left), QKeySequence(Qt::KeypadModifier | Qt::Key_4)})
			QCOMPARE(deck::shortcutSequence(deck::shortcutBinding(sequence)), sequence);
		QVERIFY(deck::shortcutBinding(QKeySequence(Qt::Key_MediaPlay)).isEmpty());
		DemoBackend b;
		deck::DeckWidget w(b);
		auto config = b.demo();
		config.hotkeys.insert("a", QJsonArray{deck::shortcutBinding(QKeySequence(Qt::Key_F8)),
			deck::shortcutBinding(QKeySequence(Qt::Key_F9))});
		config.hotkeys.insert("b", QJsonArray{deck::shortcutBinding(QKeySequence(Qt::CTRL | Qt::Key_F7))});
		w.load(config); w.resize(1000, 600); w.show(); QApplication::processEvents();
		auto *tiles = w.findChild<QListWidget *>("sceneTiles");
		QTimer::singleShot(0, [&] {
			auto *dialog = w.findChild<QDialog *>("buttonEditor"); QVERIFY(dialog);
			auto *shortcut = dialog->findChild<QKeySequenceEdit *>("buttonShortcut");
			auto *lock = dialog->findChild<QPushButton *>("shortcutLock");
			QVERIFY(lock->isChecked()); QVERIFY(!shortcut->isEnabled());
			QCOMPARE(shortcut->keySequence(), QKeySequence(Qt::Key_F8));
			lock->click(); QVERIFY(shortcut->isEnabled());
			shortcut->setKeySequence(QKeySequence(Qt::CTRL | Qt::Key_F7));
			QVERIFY(dialog->findChild<QLabel *>("shortcutWarning")->text().contains("altre botó"));
			dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
		});
		QMetaObject::invokeMethod(tiles, "itemDoubleClicked", Qt::DirectConnection,
			Q_ARG(QListWidgetItem *, tiles->item(0)));
		auto bindings = w.config().hotkeys["a"].toArray(); QCOMPARE(bindings.size(), 2);
		QCOMPARE(deck::shortcutSequence(bindings[0].toObject()), QKeySequence(Qt::CTRL | Qt::Key_F7));
		QCOMPARE(deck::shortcutSequence(bindings[1].toObject()), QKeySequence(Qt::Key_F9));
		QCOMPARE(b.saved.hotkeys, w.config().hotkeys); QCOMPARE(b.executions, 0);
		const auto before = w.config().json();
		QTimer::singleShot(0, [&] {
			auto *dialog = w.findChild<QDialog *>("buttonEditor"); QVERIFY(dialog);
			dialog->findChild<QPushButton *>("shortcutLock")->click();
			dialog->findChild<QKeySequenceEdit *>("buttonShortcut")->setKeySequence(QKeySequence(Qt::Key_F10));
			dialog->reject();
		});
		QMetaObject::invokeMethod(tiles, "itemDoubleClicked", Qt::DirectConnection,
			Q_ARG(QListWidgetItem *, tiles->item(0)));
		QCOMPARE(w.config().json(), before);
		QTimer::singleShot(0, [&] {
			auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget()); QVERIFY(menu);
			for (auto *action : menu->actions()) if (action->text() == "Graella i mida…") {
				QTimer::singleShot(0, [&] {
					auto *dialog = w.findChild<QDialog *>("gridSettings"); QVERIFY(dialog);
					dialog->findChild<QCheckBox *>("autoFit")->setChecked(true);
					dialog->findChild<QSpinBox *>("tilePixels")->setValue(160);
					QVERIFY(w.config().autoFit); QCOMPARE(w.config().tileSize, 160);
					QCOMPARE(b.saved.json(), before);
					dialog->reject();
				});
				action->trigger(); menu->close(); break;
			}
		});
		QMetaObject::invokeMethod(tiles, "customContextMenuRequested", Qt::DirectConnection, Q_ARG(QPoint, QPoint(10, 10)));
		QCOMPARE(w.config().json(), before); QCOMPARE(b.saved.json(), before);
	}
	void actualRightClickAndModeRoundtrip()
	{
		DemoBackend b;
		deck::DeckWidget w(b); w.load(b.demo()); w.resize(1000, 600); w.show();
		QApplication::processEvents();
		auto *tiles = w.findChild<QListWidget *>("sceneTiles");
		auto *lock = w.findChild<QPushButton *>("lockDeck");
		lock->setChecked(true);
		QVERIFY(w.findChild<QLabel *>("operationMode")->text().startsWith("REALITZACIÓ"));
		const auto pos = tiles->visualItemRect(tiles->item(1)).center();
		QContextMenuEvent nativeContext(QContextMenuEvent::Mouse, pos, tiles->viewport()->mapToGlobal(pos));
		QApplication::sendEvent(tiles->viewport(), &nativeContext);
		QVERIFY(!QApplication::activePopupWidget());
		QTimer::singleShot(20, [&] {
			auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget()); QVERIFY(menu);
			QCOMPARE(menu->actions().size(), 1);
			if (!qEnvironmentVariableIsEmpty("DECK_CAPTURE_DIR"))
				menu->grab().save(qEnvironmentVariable("DECK_CAPTURE_DIR") + "/menu-realitzacio.png");
			menu->actions().first()->trigger(); menu->close();
		});
		QTest::mouseClick(tiles->viewport(), Qt::RightButton, Qt::NoModifier, pos);
		QVERIFY(!w.config().locked); QVERIFY(w.findChild<QPushButton *>("editMode")->isChecked());
		QVERIFY(w.findChild<QLabel *>("operationMode")->text().startsWith("CONFIGURACIÓ"));
		QVERIFY(!w.findChild<QPushButton *>("autoTransition")->property("operational").toBool());
		QCOMPARE(b.executions, 0);
		QTimer::singleShot(20, [&] {
			auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget()); QVERIFY(menu);
			bool found = false;
			for (auto *action : menu->actions()) if (action->text() == "Editar botó…") {
				found = true;
				QTimer::singleShot(0, [&] {
					auto *editor = w.findChild<QDialog *>("buttonEditor"); QVERIFY(editor); editor->reject();
				});
				action->trigger(); menu->close(); break;
			}
			QVERIFY(found);
		});
		QTest::mouseClick(tiles->viewport(), Qt::RightButton, Qt::NoModifier, pos);
		QCOMPARE(b.executions, 0);
		QTimer::singleShot(20, [&] {
			auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget()); QVERIFY(menu);
			for (auto *action : menu->actions()) if (action->text() == "Entrar en realització") {
				action->trigger(); menu->close(); break;
			}
		});
		QTest::mouseClick(tiles->viewport(), Qt::RightButton, Qt::NoModifier, pos);
		QVERIFY(w.config().locked); QVERIFY(!w.findChild<QPushButton *>("editMode")->isChecked());
		QVERIFY(w.findChild<QPushButton *>("autoTransition")->isEnabled());
		QTest::mouseClick(tiles->viewport(), Qt::LeftButton, Qt::NoModifier, pos);
		QCOMPARE(b.executions, 1); QCOMPARE(b.state.preview, QString("b"));
		auto persisted = w.config(); w.load(persisted);
		QVERIFY(w.config().locked); QVERIFY(!w.findChild<QPushButton *>("editMode")->isChecked());
#ifdef Q_OS_MACOS
		QTimer::singleShot(20, [&] {
			auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget()); QVERIFY(menu);
			menu->actions().first()->trigger(); menu->close();
		});
		QTest::mouseClick(tiles->viewport(), Qt::LeftButton, Qt::MetaModifier, pos);
		QVERIFY(!w.config().locked); QCOMPARE(b.executions, 1);
		persisted = w.config(); w.load(persisted);
		QVERIFY(w.findChild<QPushButton *>("editMode")->isChecked());
#endif
	}
	void configurableTransitionControls()
	{
		DemoBackend b; deck::DeckWidget w(b); w.load(b.demo()); w.resize(1000, 650); w.show();
		QApplication::processEvents();
		auto *autoButton = w.findChild<QPushButton *>("autoTransition");
		auto *cutButton = w.findChild<QPushButton *>("cutTransition");
		QVERIFY(autoButton->isEnabled()); QVERIFY(!autoButton->property("operational").toBool());
		QTest::mouseClick(autoButton, Qt::LeftButton, Qt::NoModifier, autoButton->rect().center());
		QCOMPARE(b.triggers, 0);
		QImage image(90, 180, QImage::Format_RGB32); image.fill(Qt::white);
		QMimeData mime; mime.setImageData(image);
		const QPoint center = autoButton->rect().center();
		QDragEnterEvent enter(center, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
		QApplication::sendEvent(autoButton, &enter); QVERIFY(enter.isAccepted());
		QDropEvent drop(QPointF(center), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
		QApplication::sendEvent(autoButton, &drop); QVERIFY(drop.isAccepted());
		QVERIFY(!w.config().controlImages["@auto"].toString().isEmpty()); QCOMPARE(b.triggers, 0);
		QTimer::singleShot(20, [&] {
			auto *dialog = w.findChild<QDialog *>("controlEditor"); QVERIFY(dialog);
			auto *shortcut = dialog->findChild<QKeySequenceEdit *>("controlShortcut");
			QVERIFY(!shortcut->isEnabled());
			dialog->findChild<QPushButton *>("controlShortcutLock")->click(); QVERIFY(shortcut->isEnabled());
			QKeyEvent press(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
			QKeyEvent release(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier);
			QApplication::sendEvent(shortcut, &press); QApplication::sendEvent(shortcut, &release);
			QCOMPARE(shortcut->keySequence(), QKeySequence(Qt::Key_Return));
			if (!qEnvironmentVariableIsEmpty("DECK_CAPTURE_DIR"))
				dialog->grab().save(qEnvironmentVariable("DECK_CAPTURE_DIR") + "/editor-control-transicio.png");
			dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
		});
		QTest::mouseClick(autoButton, Qt::RightButton, Qt::NoModifier, center);
		QCOMPARE(deck::shortcutSequence(w.config().hotkeys["@auto"].toArray()[0].toObject()), QKeySequence(Qt::Key_Return));
		QTimer::singleShot(20, [&] {
			auto *dialog = w.findChild<QDialog *>("controlEditor"); QVERIFY(dialog);
			dialog->findChild<QPushButton *>("controlShortcutLock")->click();
			dialog->findChild<QKeySequenceEdit *>("controlShortcut")->setKeySequence(QKeySequence(Qt::SHIFT | Qt::Key_Return));
			dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
		});
		QTest::mouseClick(cutButton, Qt::RightButton, Qt::NoModifier, cutButton->rect().center());
		QCOMPARE(deck::shortcutSequence(w.config().hotkeys["@cut"].toArray()[0].toObject()), QKeySequence(Qt::SHIFT | Qt::Key_Return));
		const auto original = w.config().json();
		QTimer::singleShot(20, [&] { auto *dialog = w.findChild<QDialog *>("controlEditor"); QVERIFY(dialog); dialog->reject(); });
		QTest::mouseClick(cutButton, Qt::RightButton, Qt::NoModifier, cutButton->rect().center());
		QCOMPARE(w.config().json(), original);
		deck::Config restored; QString error;
		QVERIFY(deck::Config::parse(original, restored, error));
		QCOMPARE(restored.controlImages, w.config().controlImages); QCOMPARE(restored.hotkeys, w.config().hotkeys);
		QCOMPARE(restored.presets[0].id, w.config().presets[0].id);
		auto invalid = original; invalid["controlImages"] = QJsonObject{{"@auto", "invalid"}};
		QVERIFY(!deck::Config::parse(invalid, restored, error));
		w.activateButton("@auto"); QCOMPARE(b.triggers, 0);
		w.findChild<QPushButton *>("lockDeck")->setChecked(true);
		QVERIFY(autoButton->property("operational").toBool());
		w.activateButton("@transition:Cortineta"); QCOMPARE(b.state.transition, QString("Cortineta")); QCOMPARE(b.triggers, 0);
		w.activateButton("@preset:" + w.config().presets[1].id); QCOMPARE(b.state.duration, 1000); QCOMPARE(b.triggers, 0);
		w.activateButton("@auto"); QCOMPARE(b.triggers, 1);
		w.activateButton("@cut"); QCOMPARE(b.triggers, 2);
		b.state.studio = false; w.activateButton("@auto"); w.activateButton("@cut"); QCOMPARE(b.triggers, 2);
		b.state.busy = true; w.activateButton("@transition:Fos"); QCOMPARE(b.state.transition, QString("Fos"));
	}
	void outputStartEntersLiveWithoutPermanentLock()
	{
		DemoBackend b; deck::DeckWidget w(b); w.load(b.demo()); w.resize(1000, 650); w.show();
		QApplication::processEvents();
		auto *lock = w.findChild<QPushButton *>("lockDeck");
		QVERIFY(!w.config().locked);
		w.outputStarting(); QVERIFY(w.config().locked);
		QVERIFY(w.findChild<QPushButton *>("autoTransition")->property("operational").toBool());
		QApplication::processEvents(); QVERIFY(b.saved.locked);
		lock->setChecked(false); QApplication::processEvents(); w.refresh();
		QVERIFY(!w.config().locked); QVERIFY(!b.saved.locked);
		w.outputStarting(); QVERIFY(w.config().locked);
		lock->setChecked(false); QApplication::processEvents(); QVERIFY(!w.config().locked);
		// Starting from a live grid preview cancels edits and must not restore editing mode.
		const int columns = w.config().columns, rows = w.config().rows;
		auto *tiles = w.findChild<QListWidget *>("sceneTiles");
		QTimer::singleShot(0, [&] {
			auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget()); QVERIFY(menu);
			for (auto *action : menu->actions()) if (action->text() == "Graella i mida…") {
				QTimer::singleShot(0, [&] {
					auto *dialog = w.findChild<QDialog *>("gridSettings"); QVERIFY(dialog);
					dialog->findChild<QSpinBox *>("gridColumns")->setValue(5);
					QVERIFY(w.config().columns == 5);
					w.outputStarting(); QVERIFY(!dialog->isVisible());
				});
				action->trigger(); menu->close(); break;
			}
		});
		QMetaObject::invokeMethod(tiles, "customContextMenuRequested", Qt::DirectConnection, Q_ARG(QPoint, QPoint(10, 10)));
		QVERIFY(w.config().locked); QCOMPARE(w.config().columns, columns); QCOMPARE(w.config().rows, rows);
		QApplication::processEvents(); QVERIFY(b.saved.locked);
		lock->setChecked(false); w.refresh(); QVERIFY(!w.config().locked);
		QCOMPARE(b.triggers, 0); QCOMPARE(b.executions, 0);
	}
	void automaticFitAcrossAspectRatios()
	{
		DemoBackend b;
		deck::DeckWidget w(b);
		auto config = b.demo(); config.autoFit = true;
		w.load(config); w.show();
		auto *tiles = w.findChild<QListWidget *>("sceneTiles");
		auto *scroll = w.findChild<QScrollArea *>("gridScroll");
		for (const QSize &size : {QSize(1000, 400), QSize(500, 900), QSize(1200, 800)}) {
			w.resize(size); QApplication::processEvents(); QApplication::processEvents();
			const auto available = scroll->viewport()->size();
			QVERIFY(tiles->width() <= available.width()); QVERIFY(tiles->height() <= available.height());
			const auto first = tiles->visualItemRect(tiles->item(0));
			QCOMPARE(first.width(), first.height());
			QVERIFY(tiles->visualItemRect(tiles->item(11)).bottom() < tiles->viewport()->height());
			QCOMPARE(tiles->visualItemRect(tiles->item(3)).y(), first.y());
			QVERIFY(tiles->visualItemRect(tiles->item(4)).y() > first.y());
			QVERIFY(tiles->width() + config.columns > available.width() ||
			        tiles->height() + config.rows > available.height());
		}
		QCOMPARE(w.config().tileSize, config.tileSize); QCOMPARE(b.executions, 0);
		config.autoFit = false; w.load(config); QApplication::processEvents();
		QCOMPARE(tiles->gridSize().width(), config.tileSize);
	}
	void transitionScrubDoesNotTakeProgram()
	{
		DemoBackend b;
		deck::DeckWidget w(b);
		w.load(b.demo());
		w.findChild<QPushButton *>("lockDeck")->click();
		w.resize(1000, 650);
		w.show();
		QApplication::processEvents();
		auto *autoButton = w.findChild<QPushButton *>("autoTransition");
		QPoint center = autoButton->rect().center();
		QPoint global = autoButton->mapToGlobal(center);
		QMouseEvent press(QEvent::MouseButtonPress, QPointF(center), QPointF(global), Qt::LeftButton,
				  Qt::LeftButton, Qt::NoModifier);
		QApplication::sendEvent(autoButton, &press);
		QMouseEvent move(QEvent::MouseMove, QPointF(center - QPoint(0, 12)), QPointF(global - QPoint(0, 12)),
				 Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
		QApplication::sendEvent(autoButton, &move);
		QMouseEvent release(QEvent::MouseButtonRelease, QPointF(center - QPoint(0, 12)),
				    QPointF(global - QPoint(0, 12)), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
		QApplication::sendEvent(autoButton, &release);
		QCOMPARE(b.state.duration, 420);
		QCOMPARE(b.triggers, 0);
		QCOMPARE(b.state.program, QString("a"));
		const auto choices = w.findChildren<QPushButton *>("transitionChoice");
		QCOMPARE(choices.size(), b.state.transitions.size());
		for (auto *choice : choices)
			if (choice->property("transitionName").toString() == "Cortineta")
				choice->click();
		QCOMPARE(b.state.transition, QString("Cortineta"));
		QCOMPARE(b.triggers, 0);
	}
};
int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	DeckTests t;
	try {
		t.typographyOverridesHostTheme();
		t.compactTransitionAndDskToggle();
		t.transitionColumnsLayout();
		t.transitionActionStaysVisibleAcrossFocusAndBusyChanges();
		t.editingShortcutBadges();
		t.illuminatedNumbersAndVisibility();
		t.dskSelectionDiffersFromFocus();
		t.portableRoundtrip();
		t.invalidImportIsAtomic();
		t.studioAndPresetInteraction();
		t.editAndLifecycleInterlocks();
		t.incrementalLayout();
		t.dropImageEmbedsWithoutExecuting();
		t.slotsDragAssignmentAndLock();
		t.transitionScrubDoesNotTakeProgram();
		t.gridAndContextualConfiguration();
		t.automaticFitAcrossAspectRatios();
		t.outputStartEntersLiveWithoutPermanentLock();
		t.configurableTransitionControls();
		t.actualRightClickAndModeRoundtrip();
		t.shortcutEditorAndLivePreviewCancellation();
		std::cout << "All test groups passed\n";
		return 0;
	} catch (const std::exception &e) {
		std::cerr << "FAILED: " << e.what() << "\n";
		return 1;
	}
}
