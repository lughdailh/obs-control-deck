#pragma once
#include <QJsonObject>
#include <QKeySequence>
#include <QMap>

namespace deck {
// OBS persists symbolic keys, so bindings remain portable across Mac and Windows.
inline const QMap<int, QString> &shortcutKeys()
{
	static const QMap<int, QString> keys = {
		{Qt::Key_Return, "RETURN"}, {Qt::Key_Enter, "ENTER"}, {Qt::Key_Escape, "ESCAPE"},
		{Qt::Key_Tab, "TAB"}, {Qt::Key_Backtab, "BACKTAB"}, {Qt::Key_Backspace, "BACKSPACE"},
		{Qt::Key_Insert, "INSERT"}, {Qt::Key_Delete, "DELETE"}, {Qt::Key_Pause, "PAUSE"},
		{Qt::Key_Print, "PRINT"}, {Qt::Key_Home, "HOME"}, {Qt::Key_End, "END"},
		{Qt::Key_Left, "LEFT"}, {Qt::Key_Right, "RIGHT"}, {Qt::Key_Up, "UP"}, {Qt::Key_Down, "DOWN"},
		{Qt::Key_PageUp, "PAGEUP"}, {Qt::Key_PageDown, "PAGEDOWN"}, {Qt::Key_Space, "SPACE"},
		{Qt::Key_Menu, "MENU"}, {Qt::Key_Help, "HELP"}, {Qt::Key_Plus, "PLUS"},
		{Qt::Key_Minus, "MINUS"}, {Qt::Key_Equal, "EQUAL"}, {Qt::Key_Comma, "COMMA"},
		{Qt::Key_Period, "PERIOD"}, {Qt::Key_Slash, "SLASH"}, {Qt::Key_Backslash, "BACKSLASH"},
		{Qt::Key_Semicolon, "SEMICOLON"}, {Qt::Key_Apostrophe, "APOSTROPHE"},
		{Qt::Key_BracketLeft, "BRACKETLEFT"}, {Qt::Key_BracketRight, "BRACKETRIGHT"},
		{Qt::Key_QuoteLeft, "QUOTELEFT"}, {Qt::Key_Asterisk, "ASTERISK"}};
	return keys;
}
inline QJsonObject shortcutBinding(const QKeySequence &sequence)
{
	if (sequence.isEmpty()) return {};
	const auto combination = sequence[0];
	const int key = combination.key();
	const auto modifiers = combination.keyboardModifiers();
	QString name;
	if ((key >= Qt::Key_A && key <= Qt::Key_Z) || (key >= Qt::Key_0 && key <= Qt::Key_9))
		name = QChar(key);
	else if (key >= Qt::Key_F1 && key <= Qt::Key_F35)
		name = "F" + QString::number(key - Qt::Key_F1 + 1);
	else
		name = shortcutKeys().value(key);
	if (name.isEmpty()) return {};
	if (modifiers.testFlag(Qt::KeypadModifier) && key != Qt::Key_Enter) {
		if ((key >= Qt::Key_0 && key <= Qt::Key_9) || QStringList{"EQUAL", "ASTERISK", "PLUS", "COMMA", "MINUS", "PERIOD", "SLASH"}.contains(name))
			name.prepend("NUM");
		else return {};
	}
#ifdef Q_OS_MACOS
	const bool control = modifiers.testFlag(Qt::MetaModifier), command = modifiers.testFlag(Qt::ControlModifier);
#else
	const bool control = modifiers.testFlag(Qt::ControlModifier), command = modifiers.testFlag(Qt::MetaModifier);
#endif
	return {{"key", "OBS_KEY_" + name}, {"shift", modifiers.testFlag(Qt::ShiftModifier)},
		{"alt", modifiers.testFlag(Qt::AltModifier)}, {"control", control}, {"command", command}};
}
inline QKeySequence shortcutSequence(const QJsonObject &binding)
{
	QString name = binding["key"].toString();
	if (!name.startsWith("OBS_KEY_")) return {};
	name.remove(0, 8);
	Qt::KeyboardModifiers modifiers;
	if (name.startsWith("NUM") && name != "NUMLOCK") { modifiers |= Qt::KeypadModifier; name.remove(0, 3); }
	int key = shortcutKeys().key(name, 0);
	if (name.size() == 1 && name[0].isLetterOrNumber()) key = name[0].unicode();
	if (name.startsWith('F')) {
		const int number = name.mid(1).toInt();
		if (number >= 1 && number <= 35) key = Qt::Key_F1 + number - 1;
	}
	if (!key) return {};
	if (binding["shift"].toBool()) modifiers |= Qt::ShiftModifier;
	if (binding["alt"].toBool()) modifiers |= Qt::AltModifier;
#ifdef Q_OS_MACOS
	if (binding["control"].toBool()) modifiers |= Qt::MetaModifier;
	if (binding["command"].toBool()) modifiers |= Qt::ControlModifier;
#else
	if (binding["control"].toBool()) modifiers |= Qt::ControlModifier;
	if (binding["command"].toBool()) modifiers |= Qt::MetaModifier;
#endif
	return QKeySequence(QKeyCombination(modifiers, Qt::Key(key)));
}
} // namespace deck
