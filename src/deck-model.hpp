#pragma once
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QVector>

namespace deck {
struct Button {
	QString id, type = "scene", label, target, name, channel, image;
	QString color = "#a4a8ad";
	bool crop = true;
	QJsonObject json() const;
	static Button fromJson(const QJsonObject &);
};
struct Preset {
	QString name, transition;
	int duration = 300;
	QString id;
};
struct Config {
	QVector<Button> buttons;
	QVector<Preset> presets;
	int tileSize = 112;
	int slotCount = 12;
	int columns = 4, rows = 3;
	int transitionColumnWidth = 180, transitionSettingsWidth = 180;
	bool locked = false;
	bool autoFit = false;
	bool showLabels = true, showStateText = true;
	void normalize();
	QJsonObject hotkeys;
	QJsonObject controlImages;
	QVector<Button> controlActions(const QStringList &transitions) const;
	QJsonObject json() const;
	static bool parse(const QJsonObject &, Config &, QString &error);
};
struct Scene {
	QString id, name;
};
struct Keyer {
	QString name, current;
	QStringList scenes;
	bool tied = false;
};
struct Snapshot {
	QVector<Scene> scenes;
	QVector<Keyer> keyers;
	QString preview, program, transition;
	QStringList transitions;
	QMap<QString, QString> transitionKinds;
	int duration = 300;
	bool studio = false, busy = false, ready = true;
	QString dskError, blockedReason;
};
class Backend {
public:
	virtual ~Backend() = default;
	virtual Snapshot snapshot() = 0;
	virtual bool execute(const Button &, QString &error) = 0;
	virtual bool transition(const QString &name, int ms, bool trigger, QString &error) = 0;
	virtual void save(const Config &) = 0;
	virtual void replace(const Config &c) { save(c); }
	virtual Config portableConfig(const Config &c) { return c; }
};
} // namespace deck
