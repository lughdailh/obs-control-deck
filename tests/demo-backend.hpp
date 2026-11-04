#pragma once
#include "deck-model.hpp"
#include <QBuffer>
#include <QImage>
#include <QPainter>

class DemoBackend : public deck::Backend {
public:
	deck::Snapshot state;
	deck::Config saved;
	int executions = 0, triggers = 0;
	DemoBackend()
	{
		state.scenes = {{"a", "Càmera 1"},    {"b", "Càmera 2"}, {"c", "Pla general"},
				{"d", "Presentació"}, {"e", "Vídeo"},    {"f", "Pausa"}};
		state.preview = "b";
		state.program = "a";
		state.studio = true;
		state.transition = "Fos";
		state.transitions = {"Tall", "Fos", "Cortineta"};
		state.duration = 300;
		state.keyers = {{"DSK 1", "Logotip", {"Logotip", "Rètol convidat", "Marcador"}, false}};
	}
	deck::Snapshot snapshot() override { return state; }
	bool execute(const deck::Button &b, QString &) override
	{
		++executions;
		if (b.type == "scene") {
			if (state.studio)
				state.preview = b.target;
			else
				state.program = b.target;
		} else
			state.keyers[0].current = b.type == "dsk-clear" || state.keyers[0].current == b.name ? QString{}
													     : b.name;
		return true;
	}
	bool transition(const QString &name, int ms, bool trigger, QString &) override
	{
		if (name != "@cut") {
			state.transition = name;
			state.duration = ms;
		}
		if (trigger) {
			++triggers;
			std::swap(state.program, state.preview);
		}
		return true;
	}
	void save(const deck::Config &c) override { saved = c; }
	deck::Config demo()
	{
		deck::Config c;
		for (const auto &s : state.scenes) {
			deck::Button b;
			b.id = s.id;
			b.target = s.id;
			b.name = s.name;
			b.label = s.name;
			QImage img(256, 256, QImage::Format_RGB32);
			QPainter p(&img);
			QLinearGradient g(0, 0, 256, 256);
			g.setColorAt(0, QColor("#193c52"));
			g.setColorAt(1, QColor("#6f8095"));
			p.fillRect(img.rect(), g);
			p.setPen(QColor("#c6d8e5"));
			auto f = p.font();
			f.setPixelSize(72);
			p.setFont(f);
			p.drawText(img.rect(), Qt::AlignCenter, s.id.toUpper());
			p.end();
			QByteArray data;
			QBuffer buffer(&data);
			buffer.open(QIODevice::WriteOnly);
			img.save(&buffer, "PNG");
			b.image = QString::fromLatin1(data.toBase64());
			c.buttons << b;
		}
		for (const auto &s : state.keyers[0].scenes) {
			deck::Button b;
			b.id = "dsk-" + s;
			b.type = "dsk";
			b.name = s;
			b.label = s;
			b.channel = "DSK 1";
			b.color = "#4a354c";
			c.buttons << b;
		}
		deck::Button clear;
		clear.id = "clear";
		clear.type = "dsk-clear";
		clear.channel = "DSK 1";
		clear.label = "Netejar DSK";
		c.buttons << clear;
		c.presets = {{"Fos ràpid", "Fos", 300}, {"Fos lent", "Fos", 1000}, {"Cortineta", "Cortineta", 700}};
		return c;
	}
};
