#pragma once
#include "deck-model.hpp"
#include <QJsonArray>
#include <algorithm>
#include <functional>

namespace deck {
using DskRequest = std::function<QJsonObject(const QString &, const QJsonObject &, QString &)>;

inline QVector<Keyer> readDskKeyers(const QJsonObject &data, const DskRequest &request, QString &error)
{
	QVector<Keyer> keyers;
	for (const auto &v : data["downstream_keyers"].toArray()) {
		const auto o = v.toObject();
		Keyer k;
		k.name = o["name"].toString();
		k.tied = o["tie"].toBool();
		for (const auto &entry : o["scenes"].toArray()) k.scenes << entry.toObject()["name"].toString();
		// get_downstream_keyers serializes currentItem (focus), not selectedItems.
		// dsk_get_scene reads the selection used to apply the actual DSK composition.
		const auto selected = request("dsk_get_scene", {{"view_name", ""}, {"dsk_name", k.name}}, error);
		if (!error.isEmpty()) return {};
		if (!selected["success"].toBool() || !selected["scene"].isString()) {
			error = "DSK no ha pogut confirmar la composició seleccionada.";
			return {};
		}
		k.current = selected["scene"].toString();
		keyers.push_back(k);
	}
	return keyers;
}

inline bool executeDsk(const Button &b, const QVector<Keyer> &keyers, const DskRequest &request, QString &error)
{
	const auto k =
		std::find_if(keyers.begin(), keyers.end(), [&](const Keyer &v) { return v.name == b.channel; });
	if (k == keyers.end()) {
		error = "El canal DSK no existeix.";
		return false;
	}
	if (k->tied) {
		error = "El canal DSK té TIE actiu. Desactiva’l al panell DSK abans d’utilitzar aquest botó.";
		return false;
	}
	if (b.type == "dsk" && !k->scenes.contains(b.name)) {
		error = "L’element no està afegit a aquest canal DSK.";
		return false;
	}
	// DSK's SwitchDSK uses an empty scene name to clear the channel.
	const QString next = b.type == "dsk-clear" || k->current == b.name ? QString{} : b.name;
	const auto response =
		request("dsk_select_scene", {{"view_name", ""}, {"dsk_name", b.channel}, {"scene", next}}, error);
	if (error.isEmpty() && !response["success"].toBool())
		error = "DSK no ha confirmat l’acció.";
	return error.isEmpty();
}
} // namespace deck
