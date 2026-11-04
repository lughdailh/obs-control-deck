#include "deck-model.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QImage>
#include <QSet>
#include <QUuid>
#include <algorithm>

namespace deck {
QJsonObject Button::json() const
{
	return {{"id", id},           {"type", type},   {"label", label}, {"target", target}, {"name", name},
		{"channel", channel}, {"image", image}, {"color", color}, {"crop", crop}};
}
Button Button::fromJson(const QJsonObject &o)
{
	Button b;
	b.id = o["id"].toString();
	b.type = o["type"].toString();
	b.label = o["label"].toString();
	b.target = o["target"].toString();
	b.name = o["name"].toString();
	b.channel = o["channel"].toString();
	b.image = o["image"].toString();
	b.color = o["color"].toString("#a4a8ad");
	b.crop = o["crop"].toBool(true);
	return b;
}
QJsonObject Config::json() const
{
	QJsonArray bs, ps;
	for (const auto &b : buttons)
		bs.append(b.json());
	for (const auto &p : presets)
		ps.append(QJsonObject{{"name", p.name}, {"transition", p.transition}, {"duration", p.duration}, {"id", p.id}});
	return {{"schema", 1},   {"tileSize", tileSize}, {"slotCount", slotCount}, {"locked", locked}, {"autoFit", autoFit}, {"showLabels", showLabels}, {"showStateText", showStateText}, {"columns", columns}, {"rows", rows},
		{"transitionColumnWidth", transitionColumnWidth}, {"transitionSettingsWidth", transitionSettingsWidth},
		{"buttons", bs}, {"presets", ps},        {"hotkeys", hotkeys}, {"controlImages", controlImages}};
}
void Config::normalize()
{
	for (auto &p : presets)
		if (p.id.isEmpty()) p.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	transitionColumnWidth = std::clamp(transitionColumnWidth, 140, 900);
	transitionSettingsWidth = std::clamp(transitionSettingsWidth, 170, 900);
	columns = std::clamp(columns, 1, 200);
	rows = std::clamp(rows, 1, std::max(1, 200 / columns));
	if (buttons.size() > columns * rows) {
		columns = 4;
		rows = (int(buttons.size()) + 3) / 4;
	}
	slotCount = columns * rows;
	while (buttons.size() < slotCount) {
		Button b;
		b.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
		b.type = "empty";
		buttons.push_back(b);
	}
}
QVector<Button> Config::controlActions(const QStringList &transitions) const
{
	QVector<Button> result;
	auto add = [&](const QString &id, const QString &label) {
		Button b; b.id = id; b.label = label; b.type = "control"; result.push_back(b);
	};
	add("@auto", "Transició AUTO"); add("@cut", "Tall CUT");
	QStringList names = transitions;
	for (const auto &key : hotkeys.keys() + controlImages.keys())
		if (key.startsWith("@transition:") && !names.contains(key.mid(12))) names.push_back(key.mid(12));
	for (const auto &name : names) add("@transition:" + name, "Seleccionar transició: " + name);
	for (const auto &preset : presets) add("@preset:" + preset.id, "Preset: " + preset.name);
	return result;
}
bool Config::parse(const QJsonObject &o, Config &out, QString &error)
{
	auto fail = [&](const QString &s) {
		error = s;
		return false;
	};
	if (o["schema"].toInt() != 1 || !o["buttons"].isArray() || !o["presets"].isArray())
		return fail("Format de botonera no compatible.");
	Config c;
	c.slotCount = o["slotCount"].toInt(std::max(12, int(o["buttons"].toArray().size())));
	c.locked = o["locked"].toBool(false);
	if (o.contains("autoFit") && !o["autoFit"].isBool())
		return fail("Adaptació de mida incorrecta.");
	c.autoFit = o["autoFit"].toBool(false);
	for (const auto &key : {"transitionColumnWidth", "transitionSettingsWidth"}) {
		const int minimum = QString(key) == "transitionColumnWidth" ? 140 : 170;
		if (o.contains(key) && (!o[key].isDouble() || o[key].toInt(-1) < minimum || o[key].toInt(-1) > 900 ||
		                      o[key].toDouble() != o[key].toInt(-1)))
			return fail("Amplada de columna incorrecta.");
	}
	c.transitionColumnWidth = o["transitionColumnWidth"].toInt(180);
	c.transitionSettingsWidth = o["transitionSettingsWidth"].toInt(180);
	for (const auto &key : {"showLabels", "showStateText"})
		if (o.contains(key) && !o[key].isBool()) return fail("Opció de visibilitat incorrecta.");
	c.showLabels = o["showLabels"].toBool(true);
	c.showStateText = o["showStateText"].toBool(true);
	if (c.slotCount < 1 || c.slotCount > 200 || c.slotCount < o["buttons"].toArray().size())
		return fail("Nombre de caselles incorrecte.");
	if (o.contains("columns") || o.contains("rows")) {
		c.columns = o["columns"].toInt(-1);
		c.rows = o["rows"].toInt(-1);
		if (c.columns < 1 || c.columns > 200 || c.rows < 1 || c.rows > 200 ||
		    c.columns * c.rows > 200 || c.columns * c.rows < o["buttons"].toArray().size())
			return fail("Graella incorrecta (màxim 200 caselles).");
	} else {
		c.columns = std::min(4, c.slotCount);
		c.rows = (c.slotCount + c.columns - 1) / c.columns;
	}
	if (o.contains("hotkeys") && !o["hotkeys"].isObject())
		return fail("Dreceres incorrectes.");
	c.hotkeys = o["hotkeys"].toObject();
	if (QJsonDocument(c.hotkeys).toJson(QJsonDocument::Compact).size() > 256 * 1024)
		return fail("Massa dreceres.");
	if (o.contains("controlImages") && !o["controlImages"].isObject())
		return fail("Imatges dels controls incorrectes.");
	c.controlImages = o["controlImages"].toObject();
	if (c.controlImages.size() > 256) return fail("Massa controls personalitzats.");
	for (auto it = c.controlImages.begin(); it != c.controlImages.end(); ++it) {
		if ((!QStringList{"@auto", "@cut"}.contains(it.key()) &&
		     !it.key().startsWith("@transition:") && !it.key().startsWith("@preset:")) ||
		    it.key().size() > 640 || !it.value().isString() || it.value().toString().size() > 3 * 1024 * 1024)
			return fail("Control personalitzat incorrecte.");
		if (!it.value().toString().isEmpty()) {
			QImage image;
			if (!image.loadFromData(QByteArray::fromBase64(it.value().toString().toLatin1()), "PNG") ||
			    image.width() > 1024 || image.height() > 1024) return fail("Imatge de control no vàlida.");
		}
	}
	c.tileSize = o["tileSize"].toInt(112);
	if (c.tileSize < 80 || c.tileSize > 200)
		return fail("Mida de botó fora de rang.");
	if (o["buttons"].toArray().size() > 200 || o["presets"].toArray().size() > 32)
		return fail("Massa botons o presets.");
	QSet<QString> ids;
	for (auto v : o["buttons"].toArray()) {
		if (!v.isObject())
			return fail("Botó incorrecte.");
		auto b = Button::fromJson(v.toObject());
		if (!QStringList{"empty", "scene", "dsk", "dsk-clear"}.contains(b.type))
			return fail("Tipus de botó desconegut.");
		if (b.id.startsWith("@")) return fail("Identificador de botó reservat.");
		if (b.id.isEmpty())
			b.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
		if (ids.contains(b.id))
			return fail("Identificador de botó duplicat.");
		ids.insert(b.id);
		if ((b.type != "empty" && b.label.trimmed().isEmpty()) || b.label.size() > 120 || b.name.size() > 512 ||
		    b.channel.size() > 512)
			return fail("Etiqueta o destinació incorrecta.");
		if (b.type == "scene" && b.name.isEmpty())
			return fail("Falta l’escena.");
		if (b.type.startsWith("dsk") && b.channel.isEmpty())
			return fail("Falta el canal DSK.");
		if (b.type == "dsk" && b.name.isEmpty())
			return fail("Falta l’element DSK.");
		if (b.image.size() > 3 * 1024 * 1024)
			return fail("Imatge massa gran.");
		if (!b.image.isEmpty()) {
			QImage img;
			if (!img.loadFromData(QByteArray::fromBase64(b.image.toLatin1()), "PNG") ||
			    img.width() > 1024 || img.height() > 1024)
				return fail("Imatge PNG no vàlida o massa gran.");
		}
		c.buttons.push_back(b);
	}
	QSet<QString> presetIds;
	for (auto v : o["presets"].toArray()) {
		const auto p = v.toObject();
		Preset preset{p["name"].toString(), p["transition"].toString(), p["duration"].toInt(-1)};
		preset.id = p["id"].toString();
		if (!preset.id.isEmpty() && (preset.id.size() > 120 || presetIds.contains(preset.id)))
			return fail("Identificador de preset incorrecte.");
		if (!preset.id.isEmpty()) presetIds.insert(preset.id);
		if (preset.name.trimmed().isEmpty() || preset.name.size() > 120 || preset.transition.isEmpty() ||
		    preset.duration < 50 || preset.duration > 20000)
			return fail("Preset incorrecte.");
		c.presets.push_back(preset);
	}
	c.normalize();
	out = c;
	return true;
}
} // namespace deck
