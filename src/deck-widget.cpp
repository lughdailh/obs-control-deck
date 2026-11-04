#include "deck-widget.hpp"
#include "shortcut.hpp"
#include <QKeySequenceEdit>
#include <QJsonArray>
#include <QAbstractItemModel>
#include <QApplication>
#include <QBuffer>
#include <QCheckBox>
#include <QContextMenuEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QImageReader>
#include <QDragEnterEvent>
#include <QDrag>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <functional>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QGridLayout>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QSplitterHandle>
#include <QStyledItemDelegate>
#include <QStyleOptionButton>
#include <QTimer>
#include <QElapsedTimer>
#include <cmath>
#include <QUuid>
#include <QVBoxLayout>
#include <algorithm>

namespace deck {
namespace {
constexpr int DataRole = Qt::UserRole, StateRole = Qt::UserRole + 1;
QString encodeImage(QImage image)
{
	if (image.isNull() || image.width() > 16384 || image.height() > 16384)
		return {};
	if (image.width() > 512 || image.height() > 512)
		image = image.scaled(512, 512, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	image.save(&buffer, "PNG");
	return QString::fromLatin1(bytes.toBase64());
}
QString readImage(const QString &path)
{
	QFileInfo file(path);
	if (!file.isFile() || file.size() > 32 * 1024 * 1024)
		return {};
	QImageReader reader(path);
	reader.setAutoTransform(true);
	if (!reader.canRead() || !reader.size().isValid() || reader.size().width() > 16384 ||
	    reader.size().height() > 16384)
		return {};
	reader.setScaledSize(reader.size().scaled(512, 512, Qt::KeepAspectRatio));
	return encodeImage(reader.read());
}
class ImageDropList : public QListWidget {
public:
	using QListWidget::QListWidget;
	bool locked = false, reorderEnabled = false;
	std::function<void(const QString &, const QString &)> imageDropped;
	std::function<void()> invalidImage;
	std::function<void(QObject *, int)> sourceDropped;
	std::function<void(const QString &, int)> reordered;
	std::function<void(const QPoint &)> contextRequested;
	static bool imageDrag(const QMimeData *m)
	{
		return m->hasImage() || (m->hasUrls() && m->urls().size() == 1 && m->urls().first().isLocalFile());
	}
	static bool nativeDrag(QObject *source)
	{
		auto *view = qobject_cast<QAbstractItemView *>(source);
		return view && view->objectName() == "scenes" &&
		       view->dragDropMode() == QAbstractItemView::InternalMove && view->currentIndex().isValid();
	}
	bool ownDrag(const QMimeData *m, QObject *source) const
	{
		return source == this && reorderEnabled && m->hasFormat("application/x-obs-control-deck-button");
	}
	bool allowed(const QMimeData *m, QObject *source) const
	{
		return !locked && (imageDrag(m) || nativeDrag(source) || ownDrag(m, source));
	}

protected:
	bool contextDown_ = false;
	void mousePressEvent(QMouseEvent *event) override
	{
		contextDown_ = event->button() == Qt::RightButton;
#ifdef Q_OS_MACOS
		contextDown_ = contextDown_ || (event->button() == Qt::LeftButton &&
					       event->modifiers().testFlag(Qt::MetaModifier));
#endif
		if (contextDown_) { event->accept(); return; }
		QListWidget::mousePressEvent(event);
	}
	void mouseReleaseEvent(QMouseEvent *event) override
	{
		if (contextDown_) {
			contextDown_ = false;
			const auto global = event->globalPosition().toPoint();
			event->accept();
			// Open after the native release has finished; never treat it as a scene click.
			QTimer::singleShot(0, this, [this, global] {
				if (contextRequested) contextRequested(global);
			});
			return;
		}
		QListWidget::mouseReleaseEvent(event);
	}
	void contextMenuEvent(QContextMenuEvent *event) override
	{
		if (event->reason() == QContextMenuEvent::Keyboard && contextRequested) {
			const auto rect = currentItem() ? visualItemRect(currentItem()) : viewport()->rect();
			contextRequested(viewport()->mapToGlobal(rect.center()));
		}
		event->accept();
	}
	void startDrag(Qt::DropActions) override
	{
		if (locked || !reorderEnabled || !currentItem())
			return;
		auto *drag = new QDrag(this);
		auto *mime = new QMimeData;
		mime->setData("application/x-obs-control-deck-button",
			      Button::fromJson(currentItem()->data(DataRole).toJsonObject()).id.toUtf8());
		drag->setMimeData(mime);
		drag->setPixmap(viewport()->grab(visualItemRect(currentItem())));
		drag->exec(Qt::MoveAction);
		drag->deleteLater();
	}
	void dragEnterEvent(QDragEnterEvent *e) override
	{
		if (allowed(e->mimeData(), e->source())) {
			e->setDropAction((ownDrag(e->mimeData(), e->source()) || nativeDrag(e->source()))
						 ? Qt::MoveAction
						 : Qt::CopyAction);
			e->accept();
		} else
			e->ignore();
	}
	void dragMoveEvent(QDragMoveEvent *e) override
	{
		if (allowed(e->mimeData(), e->source())) {
			e->setDropAction((ownDrag(e->mimeData(), e->source()) || nativeDrag(e->source()))
						 ? Qt::MoveAction
						 : Qt::CopyAction);
			e->accept();
		} else
			e->ignore();
	}
	void dropEvent(QDropEvent *e) override
	{
		if (!allowed(e->mimeData(), e->source())) {
			e->ignore();
			return;
		}
		auto *item = itemAt(e->position().toPoint());
		const int index = item ? row(item) : -1;
		if (ownDrag(e->mimeData(), e->source())) {
			if (reordered)
				reordered(
					QString::fromUtf8(e->mimeData()->data("application/x-obs-control-deck-button")),
					index);
			e->setDropAction(Qt::MoveAction);
			e->accept();
			return;
		}
		if (nativeDrag(e->source())) {
			if (sourceDropped)
				sourceDropped(e->source(), index);
			e->setDropAction(Qt::MoveAction);
			e->accept();
			return;
		}
		QString encoded = e->mimeData()->hasUrls()
					  ? readImage(e->mimeData()->urls().first().toLocalFile())
					  : encodeImage(qvariant_cast<QImage>(e->mimeData()->imageData()));
		if (encoded.isEmpty()) {
			if (invalidImage)
				invalidImage();
			e->ignore();
			return;
		}
		if (imageDropped)
			imageDropped(item ? Button::fromJson(item->data(DataRole).toJsonObject()).id : QString{},
				     encoded);
		e->setDropAction(Qt::CopyAction);
		e->accept();
	}
};
// Shadow follows the glyphs so the image stays visible above and below the text.
void shadowText(QPainter *p, const QRect &rect, int flags, const QString &text, const QColor &color)
{
	p->save();
	p->setBackgroundMode(Qt::TransparentMode);
	p->setPen(QColor(0, 0, 0, 150));
	for (const QPoint &offset : {QPoint(-1, 0), QPoint(1, 0), QPoint(0, -1), QPoint(0, 1)})
		p->drawText(rect.translated(offset), flags, text);
	p->setPen(QColor(0, 0, 0, 240));
	p->drawText(rect.translated(0, 2), flags, text);
	p->setPen(color);
	p->drawText(rect, flags, text);
	p->restore();
}
void capText(QPainter *p, const QRect &rect, const QString &text, const QColor &ink, bool overImage)
{
	p->save(); p->setBackgroundMode(Qt::TransparentMode);
	p->setPen(QColor(245, 246, 240, overImage ? 145 : 65));
	if (overImage) {
		for (const QPoint &offset : {QPoint(-1,0), QPoint(1,0), QPoint(0,-1), QPoint(0,1)})
			p->drawText(rect.translated(offset), Qt::AlignCenter, text);
	} else p->drawText(rect.translated(0, 1), Qt::AlignCenter, text);
	p->setPen(ink); p->drawText(rect, Qt::AlignCenter, text); p->restore();
}
QString shortcutLabel(const QJsonObject &hotkeys, const QString &id)
{
	QStringList labels;
	for (const auto &binding : hotkeys.value(id).toArray()) {
		const auto text = shortcutSequence(binding.toObject()).toString(QKeySequence::NativeText);
		if (!text.isEmpty() && !labels.contains(text)) labels.append(text);
	}
	return labels.join(" / ");
}
int paintShortcutBadge(QPainter *p, const QRect &area, const QString &text)
{
	if (text.isEmpty() || area.width() < 20) return 0;
	p->save();
	QFont font = p->font(); font.setPixelSize(10); font.setBold(true); p->setFont(font);
	const QFontMetrics metrics(font);
	const int height = metrics.height() + 4;
	const int width = std::min(area.width(), metrics.horizontalAdvance(text) + 10);
	const QRect badge(area.right() - width + 1, area.top(), width, height);
	p->setPen(QColor("#aab8cb")); p->setBrush(QColor("#17212e"));
	p->drawRoundedRect(badge.adjusted(0, 0, -1, -1), 3, 3);
	p->setPen(Qt::white);
	p->drawText(badge, Qt::AlignCenter, metrics.elidedText(text, Qt::ElideRight, width - 8));
	p->restore();
	return height;
}
// The host can repolish styles after docking. Keep three disjoint bands even
// when a QPushButton size hint changes during that repolish.
class TransitionRail : public QWidget {
public:
	QWidget *top = nullptr, *action = nullptr, *bottom = nullptr;
	QSize sizeHint() const override { return {180, 420}; }
	QSize minimumSizeHint() const override { return {170, 402}; }
	void arrange() {
		if (!top || !action || !bottom) return;
		const int actionHeight = 106, gap = 12;
		const int half = (height() - actionHeight - 2 * gap) / 2;
		top->setGeometry(0, 0, width(), half);
		action->setGeometry(0, half + gap, width(), actionHeight);
		bottom->setGeometry(0, half + gap + actionHeight + gap, width(), half);
	}
protected:
	void resizeEvent(QResizeEvent *event) override { QWidget::resizeEvent(event); arrange(); }
};
class TileDelegate : public QStyledItemDelegate {
public:
	using QStyledItemDelegate::QStyledItemDelegate;
	void paint(QPainter *p, const QStyleOptionViewItem &o, const QModelIndex &i) const override
	{
		const auto b = Button::fromJson(i.data(DataRole).toJsonObject());
		const auto state = i.data(StateRole).toString();

		const QRect r = o.rect.adjusted(7, 7, -7, -7);
		QPainterPath shape;
		shape.addRoundedRect(QRectF(r), 8, 8);
		const bool red = state == "EN DIRECTE" || state == "DIRECTE + PREVI" || state == "ACTIU";
		const bool green = state == "PREVI";
		QColor edge = red                       ? QColor("#ff4159")
			      : green                   ? QColor("#20df8a")
			      : state.contains("FALTA") ? QColor("#ffbc47")
							: QColor("#64748b");
		p->save();
		p->setRenderHint(QPainter::Antialiasing);
		p->setRenderHint(QPainter::SmoothPixmapTransform);
		p->setBrush(Qt::NoBrush);
		if (red || green)
			for (int width = 12; width >= 4; width -= 2) {
				auto color = edge;
				color.setAlpha(12 + (12 - width) * 5);
				p->setPen(QPen(color, width));
				p->drawPath(shape);
			}
		p->save();
		p->setClipPath(shape);
		QColor base(b.color);
		if (!base.isValid() || base == QColor("#374151"))
			base = QColor("#a4a8ad");
		p->fillPath(shape, base);
		QImage img;
		if (!b.image.isEmpty())
			img.loadFromData(QByteArray::fromBase64(b.image.toLatin1()), "PNG");
		if (!img.isNull()) {
			const QSize scaled = img.size().scaled(r.size(), b.crop ? Qt::KeepAspectRatioByExpanding
										: Qt::KeepAspectRatio);
			p->drawImage(QRect(r.center().x() - scaled.width() / 2, r.center().y() - scaled.height() / 2,
					   scaled.width(), scaled.height()),
				     img);
		}
		// Add warm light to the artwork itself, beneath every caption and number.
		if (red || green) {
			const qreal level = red ? 1.0 : (o.widget ? o.widget->property("previewLight").toDouble() : 1.0);
			p->save(); p->setCompositionMode(QPainter::CompositionMode_Plus);
			QRadialGradient light(r.center(), std::max(r.width(), r.height()) * .78);
			light.setColorAt(0, QColor(255, 240, 150, qRound(150 * level)));
			light.setColorAt(1, QColor(255, 218, 110, qRound(72 * level)));
			p->fillPath(shape, light); p->restore();
		}
		const bool labels = !o.widget || !o.widget->property("hideLabels").toBool();
		const bool stateText = !o.widget || !o.widget->property("hideStateText").toBool();
		const bool compact = r.height() < 44;
		QRect content = r.adjusted(4, 3, -4, -3);
		auto captionFont = o.font; captionFont.setWeight(QFont::Bold);
		captionFont.setPixelSize(std::clamp(r.height() / 8, 9, 16));
		const int captionHeight = QFontMetrics(captionFont).height() + 2;
		p->setFont(captionFont);
		const auto shortcut = o.widget && o.widget->property("configurationMode").toBool() ? i.data(Qt::UserRole + 4).toString() : QString();
		if (stateText && !compact) {
			QFont badgeFont = o.font; badgeFont.setPixelSize(10); badgeFont.setBold(true);
			const int reserve = shortcut.isEmpty() ? 0 : QFontMetrics(badgeFont).horizontalAdvance(shortcut) + 14;
			const QRect header(content.left(), content.top(), std::max(0, content.width() - reserve), captionHeight);
			shadowText(p, header, Qt::AlignCenter, QFontMetrics(captionFont).elidedText(state, Qt::ElideRight, header.width()),
				red || green ? edge : QColor(Qt::white));
			content.setTop(header.bottom() + 2);
		}
		if (labels && !compact) {
			const QRect footer(content.left(), content.bottom() - captionHeight + 1, content.width(), captionHeight);
			capText(p, footer, QFontMetrics(captionFont).elidedText(b.type == "empty" ? QString{} : b.label, Qt::ElideRight, footer.width()), red || green ? QColor("#171a1c") : QColor("#34383c"), !img.isNull() || base.lightness() < 100);
			content.setBottom(footer.top() - 2);
		}
		paintShortcutBadge(p, r.adjusted(4, 3, -4, -3), shortcut);
		// Centre the visible glyphs on the whole tile, independently of captions and badges.
		const QString number = QString::number(i.row() + 1).rightJustified(2, '0');
		auto numberFont = o.font; numberFont.setWeight(QFont::Black); numberFont.setItalic(false);
		QPainterPath digits;
		for (int pixels = std::max(1, qRound(r.height() * (r.height() < 90 ? .56 : .62))); pixels >= 1; --pixels) {
			numberFont.setPixelSize(pixels); digits = QPainterPath(); digits.addText(0, 0, numberFont, number); digits.setFillRule(Qt::WindingFill);
			if (digits.boundingRect().width() <= r.width() * .86) break;
		}
		const auto bounds = digits.boundingRect();
		digits.translate(QRectF(r).center() - bounds.center());
		const QColor ink = red || green ? QColor("#111315") : b.type == "empty" ? QColor("#60666c") : QColor("#34383c");
		if (!img.isNull() || base.lightness() < 100) p->strokePath(digits, QPen(QColor(245, 246, 240, 145), 1.5));
		p->fillPath(digits, ink);

		if (red || green) {
			auto color = edge;
			color.setAlpha(45);
			p->setBrush(Qt::NoBrush);
			p->setPen(QPen(color, 12));
			p->drawPath(shape);
		}
		p->restore();
		p->setBrush(Qt::NoBrush);
		p->setPen(QPen(edge, red || green ? 2 : 1));
		p->drawPath(shape);
		if ((o.state & QStyle::State_Selected) && !red && !green) {
			auto color = o.palette.highlight().color();
			color.setAlpha(170);
			p->setPen(QPen(color, 1.5, Qt::DashLine));
			p->drawRoundedRect(r.adjusted(3, 3, -3, -3), 6, 6);
		}
		p->restore();
	}
};
class ControlButton : public QPushButton {
public:
	using QPushButton::QPushButton;
	bool operational = false, configurable = false, contextDown = false;
	QString encodedImage;
	QImage artwork;
	std::function<void(const QPoint &)> contextRequested;
	std::function<void(const QString &)> imageDropped;
	void setArtwork(const QString &encoded) {
		if (encodedImage == encoded) return;
		encodedImage = encoded; artwork = QImage{};
		artwork.loadFromData(QByteArray::fromBase64(encoded.toLatin1()), "PNG"); update();
	}
protected:
	void paintEvent(QPaintEvent *event) override {
		const auto key = property("shortcutLabel").toString();
		if (artwork.isNull()) {
			if (key.isEmpty()) { QPushButton::paintEvent(event); return; }
			QPainter p(this); QStyleOptionButton option; initStyleOption(&option); option.text.clear();
			style()->drawControl(QStyle::CE_PushButton, &option, &p, this);
			const QRect badgeArea = rect().adjusted(3, 3, -3, -3);
			const int height = paintShortcutBadge(&p, badgeArea, key);
			p.setPen(palette().color(isEnabled() ? QPalette::Active : QPalette::Disabled, QPalette::ButtonText));
			p.drawText(rect().adjusted(2, height + 4, -2, -2), Qt::AlignCenter, text());
			return;
		}
		QPainter p(this); p.setRenderHint(QPainter::Antialiasing); p.setRenderHint(QPainter::SmoothPixmapTransform);
		QStyleOptionButton option; initStyleOption(&option); option.text.clear();
		style()->drawControl(QStyle::CE_PushButton, &option, &p, this);
		const QRect area = rect().adjusted(2, 2, -2, -2);
		QPainterPath clip; clip.addRoundedRect(area, 8, 8);
		p.setClipPath(clip);
		const QSize size = artwork.size().scaled(area.size(), Qt::KeepAspectRatioByExpanding);
		p.drawImage(QRect(area.center() - QPoint(size.width()/2, size.height()/2), size), artwork);
		if (!isEnabled()) p.fillRect(area, QColor(0,0,0,120));
		p.setFont(font()); shadowText(&p, area.adjusted(5, 3, -5, -3), Qt::AlignCenter | Qt::TextWordWrap, text(), Qt::white);
		p.setClipping(false); p.setBrush(Qt::NoBrush);
		p.setPen(QPen(isChecked() ? palette().color(QPalette::Highlight) : palette().color(QPalette::Mid), isChecked() ? 3 : 1));
		p.drawPath(clip);
		paintShortcutBadge(&p, rect().adjusted(3, 3, -3, -3), key);
	}
	void mousePressEvent(QMouseEvent *event) override {
		contextDown = event->button() == Qt::RightButton;
#ifdef Q_OS_MACOS
		contextDown = contextDown || (event->button() == Qt::LeftButton && event->modifiers().testFlag(Qt::MetaModifier));
#endif
		if (contextDown || !operational) { event->accept(); return; }
		QPushButton::mousePressEvent(event);
	}
	void mouseReleaseEvent(QMouseEvent *event) override {
		if (contextDown) {
			contextDown = false; const auto global = event->globalPosition().toPoint(); event->accept();
			QTimer::singleShot(0, this, [this, global] { if (contextRequested) contextRequested(global); }); return;
		}
		if (!operational) { event->accept(); return; }
		QPushButton::mouseReleaseEvent(event);
	}
	void contextMenuEvent(QContextMenuEvent *event) override {
		if (event->reason() == QContextMenuEvent::Keyboard && contextRequested)
			contextRequested(mapToGlobal(rect().center()));
		event->accept();
	}
	void dragEnterEvent(QDragEnterEvent *event) override {
		if (configurable && ImageDropList::imageDrag(event->mimeData())) event->acceptProposedAction();
		else event->ignore();
	}
	void dropEvent(QDropEvent *event) override {
		if (!configurable || !ImageDropList::imageDrag(event->mimeData())) { event->ignore(); return; }
		const auto encoded = event->mimeData()->hasUrls() ? readImage(event->mimeData()->urls().first().toLocalFile()) :
			encodeImage(qvariant_cast<QImage>(event->mimeData()->imageData()));
		if (encoded.isEmpty()) { event->ignore(); return; }
		event->acceptProposedAction(); if (imageDropped) imageDropped(encoded);
	}
};
class ScrubButton : public ControlButton {
public:
	using ControlButton::ControlButton;
	std::function<int()> duration;
	std::function<void(int)> scrub;

private:
	QPoint press_;
	int start_ = 300, last_ = 300;
	bool dragging_ = false;

protected:
	void mousePressEvent(QMouseEvent *e) override
	{
		if (e->button() == Qt::LeftButton) {
			press_ = e->globalPosition().toPoint();
			start_ = duration ? duration() : 300;
			last_ = start_;
			dragging_ = false;
		}
		ControlButton::mousePressEvent(e);
	}
	void mouseMoveEvent(QMouseEvent *e) override
	{
		if (operational && !contextDown && (e->buttons() & Qt::LeftButton)) {
			int dy = press_.y() - e->globalPosition().toPoint().y();
			if (std::abs(dy) >= 5)
				dragging_ = true;
			if (dragging_) {
				int next = std::clamp(start_ + dy * 10, 50, 20000);
				if (next != last_) {
					last_ = next;
					if (scrub)
						scrub(next);
				}
				e->accept();
				return;
			}
		}
		QPushButton::mouseMoveEvent(e);
	}
	void mouseReleaseEvent(QMouseEvent *e) override
	{
		if (dragging_ && e->button() == Qt::LeftButton) {
			dragging_ = false;
			setDown(false);
			e->accept();
			return;
		}
		ControlButton::mouseReleaseEvent(e);
	}
};
bool isCutTransition(const QString &name, const QString &kind = {})
{
	if (!kind.isEmpty()) return kind == "cut_transition";
	const auto value = name.trimmed();
	return value.compare("Tall", Qt::CaseInsensitive) == 0 || value.compare("Talla", Qt::CaseInsensitive) == 0 ||
	       value.compare("Cut", Qt::CaseInsensitive) == 0 || value.compare("Corte", Qt::CaseInsensitive) == 0;
}
QString compactTransitionCode(const QString &name, const QString &kind = {})
{
	const auto value = name.trimmed();
	if (isCutTransition(value, kind))
		return "C";
	if (kind == "fade_transition") return "F";
	if (kind == "move_transition") return "M";
	if (!kind.isEmpty()) return value.left(1).toUpper();
	if (value.startsWith("esva", Qt::CaseInsensitive) || value.compare("Fos", Qt::CaseInsensitive) == 0 ||
	    value.compare("Fade", Qt::CaseInsensitive) == 0)
		return "F";
	if (value.startsWith("mov", Qt::CaseInsensitive)) return "M";
	return value.left(1).toUpper();
}
QPushButton *button(const QString &s, QLayout *layout, const QString &id = {})
{
	auto *b = new QPushButton(s);
	b->setObjectName(id);
	layout->addWidget(b);
	return b;
}
} // namespace

DeckWidget::DeckWidget(Backend &backend, QWidget *parent) : QWidget(parent), backend_(backend)
{
	setObjectName("controlDeck");
	const bool hasMontserrat = QFontDatabase::families().contains("Montserrat", Qt::CaseInsensitive);
	if (hasMontserrat) {
		auto deckFont = font();
		deckFont.setFamily("Montserrat");
		setFont(deckFont);
	}
	setMinimumSize(480, 300);
	// A local stylesheet takes precedence over the font imposed by the OBS theme.
	const QString typography = hasMontserrat
		? QStringLiteral("QWidget#controlDeck, QWidget#controlDeck QWidget { font-family: 'Montserrat'; }")
		: QString();
	setStyleSheet(typography +
		"QWidget#controlDeck QPushButton { min-height:24px; max-height:10000px; padding:5px 8px; border:1px solid palette(mid); border-radius:8px; background:palette(button); color:palette(button-text); }"
		"QWidget#controlDeck QPushButton:hover { background:palette(midlight); }"
		"QWidget#controlDeck QPushButton:checked { background:palette(highlight); color:palette(highlighted-text); }"
		"QWidget#controlDeck QPushButton:disabled { color:palette(mid); }"
		"QWidget#controlDeck QPushButton#autoTransition { min-height:104px; max-height:104px; margin:0px; padding:0px; background:#e4a526; color:#191919; font-weight:bold; font-size:18px; }"
		"QWidget#controlDeck QPushButton#autoTransition:disabled { background:#555; color:#aaa; }"
		"QWidget#controlDeck QPushButton#transitionChoice { min-height:34px; max-height:34px; min-width:26px; max-width:10000px; padding:0px; margin:0px; background:#252a32; border:1px solid #454d59; color:#aab0bb; font-size:10px; }"
		"QWidget#controlDeck QPushButton#transitionChoice:checked { background:#3a424f; border-color:#858f9d; color:#e1e4e8; }"
		"QWidget#controlDeck QPushButton#transitionChoice:hover { background:#303741; }"
		"QWidget#controlDeck QPushButton#transitionPreset { min-height:36px; max-height:36px; padding:0px; margin:0px; font-weight:600; }"
		"QWidget#controlDeck QPushButton#durationChoice { min-height:24px; max-height:24px; min-width:0px; margin:0px; padding:0px; font-size:10px; border-radius:5px; color:#d5dce6; background:#252a32; }"
		"QWidget#controlDeck QPushButton#durationChoice:checked { background:#3d5d87; border-color:#8bb6e7; color:#ffffff; }"
		"QWidget#controlDeck QPushButton#durationChoice:disabled { color:#8993a3; background:#252a32; }"
		"QWidget#controlDeck QPushButton#durationChoice:checked:disabled { background:#34465f; color:#d5dce6; }"
		"QWidget#controlDeck QSpinBox#transitionDuration { min-height:30px; max-height:30px; margin:0px; padding:0px 8px; border:1px solid palette(mid); border-radius:6px; }"
		"QWidget#controlDeck QListWidget::item { padding:0px; margin:0px; border:0px; }");

	auto *root = new QVBoxLayout(this);
	root->setContentsMargins(10, 10, 10, 10);
	root->setSpacing(8);
	// Commands live in the contextual menu; no editing toolbar occupies the deck.
	add_ = new QPushButton(this); add_->setObjectName("addButton"); add_->hide();
	edit_ = new QPushButton(this); edit_->setObjectName("editMode"); edit_->setCheckable(true); edit_->hide();
	lock_ = new QPushButton(this); lock_->setObjectName("lockDeck"); lock_->setCheckable(true); lock_->hide();
	presetsButton_ = new QPushButton(this); presetsButton_->hide();
	connect(lock_, &QPushButton::toggled, this, [this](bool locked) {
		config_.locked = locked;
		applyLock(); persist(); refresh();
		message(locked ? "Mode realització" : "Configuració · Arrossega per ordenar · Clic dret per editar");
	});
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QWidget::customContextMenuRequested, this, [this](QPoint pos) { contextMenu(mapToGlobal(pos)); });
	mode_ = new QLabel;
	mode_->setObjectName("operationMode");
	mode_->setWordWrap(true);
	root->addWidget(mode_);
	columns_ = new QSplitter(Qt::Horizontal);
	columns_->setObjectName("deckColumns");
	columns_->setChildrenCollapsible(false);
	columns_->setHandleWidth(8);
	auto *sceneColumn = new QWidget;
	sceneColumn->setObjectName("sceneColumn");
	sceneColumn->setMinimumWidth(100);
	auto *left = new QVBoxLayout(sceneColumn);
	left->setContentsMargins(0, 0, 0, 0);
	empty_ = new QLabel("Afegeix les escenes i els elements DSK que vulguis controlar.");
	empty_->setWordWrap(true);
	left->addWidget(empty_);
	auto *imageTiles = new ImageDropList;
	tiles_ = imageTiles;
	imageTiles->contextRequested = [this](const QPoint &global) { contextMenu(global); };
	tiles_->setObjectName("sceneTiles");
	tiles_->setViewMode(QListView::IconMode);
	tiles_->setAcceptDrops(true);
	tiles_->viewport()->setAcceptDrops(true);
	tiles_->setToolTip("Arrossega una imatge sobre un botó per personalitzar-lo. No cal redimensionar-la.");
	imageTiles->imageDropped = [this](const QString &id, const QString &image) {
		if (config_.locked || !backend_.snapshot().ready) {
			message("La botonera està bloquejada o encara s’està carregant.");
			return;
		}
		int slot = -1;
		for (int i = 0; i < config_.buttons.size(); ++i)
			if (config_.buttons[i].id == id)
				slot = i;
		if (slot < 0)
			slot = freeSlot();
		if (slot < 0) {
			message("No hi ha caselles lliures. Augmenta el nombre de caselles.");
			return;
		}
		if (config_.buttons[slot].type == "empty") {
			editButton(slot, image);
			return;
		}
		for (auto &b : config_.buttons)
			if (b.id == id) {
				b.image = image;
				break;
			}
		for (int i = 0; i < tiles_->count(); ++i) {
			auto *item = tiles_->item(i);
			auto b = Button::fromJson(item->data(DataRole).toJsonObject());
			if (b.id == id) {
				b.image = image;
				item->setData(DataRole, b.json());
				break;
			}
		}
		persist();
		message("Imatge integrada al botó. No s’ha executat cap acció.");
	};
	imageTiles->sourceDropped = [this](QObject *source, int index) {
		assignFromSource(source, index);
	};
	imageTiles->reordered = [this](const QString &id, int target) {
		moveButton(id, target);
	};
	imageTiles->invalidImage = [this] {
		message("No s’ha pogut llegir la imatge. Prova un PNG, JPG o un altre format compatible (màxim 32 MB).");
	};
	tiles_->setResizeMode(QListView::Adjust);
	tiles_->setMovement(QListView::Static);
	tiles_->setSpacing(0);
	tiles_->setDragDropMode(QAbstractItemView::DropOnly);
	tiles_->setAcceptDrops(true);
	tiles_->viewport()->setAcceptDrops(true);
	tiles_->setWrapping(true);
	tiles_->setSelectionMode(QAbstractItemView::SingleSelection);
	tiles_->setItemDelegate(new TileDelegate(tiles_));
	tiles_->setContextMenuPolicy(Qt::DefaultContextMenu);
	auto *gridScroll = new QScrollArea;
	gridScroll_ = gridScroll;
	gridScroll->setObjectName("gridScroll");
	gridScroll->setWidget(tiles_);
	gridScroll->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	gridScroll->setFrameShape(QFrame::NoFrame);
	gridScroll->viewport()->installEventFilter(this);
	tiles_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	tiles_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	left->addWidget(gridScroll);
	columns_->addWidget(sceneColumn);
	auto *railContents = new TransitionRail;
	railContents->setObjectName("transitionColumn");
	railContents->setMinimumHeight(402);
	auto *top = new QWidget(railContents);
	auto *topLayout = new QVBoxLayout(top);
	topLayout->setContentsMargins(0, 0, 0, 0);
	topLayout->setSpacing(6);
	topLayout->setAlignment(Qt::AlignTop);
	railContents->top = top;
	auto *scrubAuto = new ScrubButton("TRANSICIÓ");
	auto_ = scrubAuto;
	auto_->setObjectName("autoTransition");
	auto_->setParent(railContents);
	railContents->action = auto_;
	scrubAuto->duration = [this] {
		return duration_->value();
	};
	scrubAuto->scrub = [this](int ms) {
		scrubTransition(transition_->currentText(), ms);
	};
	auto_->setMinimumWidth(136);
	auto_->ensurePolished();
	auto_->setFixedHeight(106);
	auto_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	// Retain existing CUT shortcuts as a command without a visible second button.
	cut_ = new ControlButton("CUT", this);
	cut_->setObjectName("cutTransition"); cut_->hide();
	bindControl(auto_, "@auto"); bindControl(cut_, "@cut");

	transitionChoices_ = new QWidget;
	transitionChoices_->setObjectName("transitionChoices");
	transitionChoices_->setLayout(new QGridLayout);
	transitionChoices_->layout()->setContentsMargins(0, 0, 0, 4);
	transitionChoices_->layout()->setSpacing(4);
	transitionChoices_->layout()->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	transitionChoices_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	topLayout->addWidget(transitionChoices_);
	transition_ = new QComboBox(this);
	transition_->setObjectName("transitionType");
	transition_->setMaximumWidth(180);
	transition_->hide();
	duration_ = new QSpinBox;
	duration_->setObjectName("transitionDuration");
	duration_->setRange(50, 20000);
	duration_->setSuffix(" ms");
	duration_->setSingleStep(50);
	duration_->setToolTip("Arrossega AUTO o un botó de transició amunt o avall per ajustar els mil·lisegons.");
	duration_->hide(); // Remains the shared value for scrubbing and custom durations.
	auto *timeLabel = new QLabel("Temps (ms)");
	topLayout->addWidget(timeLabel);
	durationChoices_ = new QWidget;
	durationChoices_->setObjectName("durationChoices");
	auto *timeGrid = new QGridLayout(durationChoices_);
	timeGrid->setContentsMargins(0, 0, 0, 0);
	timeGrid->setSpacing(4);
	for (int i = 0; i < 8; ++i) {
		auto *choice = new QPushButton;
		choice->setObjectName("durationChoice");
		choice->setCheckable(true);
		choice->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
		choice->ensurePolished(); choice->setFixedHeight(26);
		timeGrid->addWidget(choice, i / 4, i % 4);
		timeGrid->setColumnStretch(i % 4, 1);
		timeGrid->setRowMinimumHeight(i / 4, 26);
		connect(choice, &QPushButton::clicked, this, [this, choice] {
			const auto snap = backend_.snapshot();
			if (!config_.locked || !snap.ready || snap.busy ||
			    isCutTransition(snap.transition, snap.transitionKinds.value(snap.transition))) return;
			QString error;
			if (!backend_.transition(snap.transition, choice->property("milliseconds").toInt(), false, error)) message(error);
			refresh();
		});
	}
	topLayout->addWidget(durationChoices_);
	auto *bottom = new QWidget(railContents);
	auto *bottomLayout = new QVBoxLayout(bottom);
	bottomLayout->setContentsMargins(0, 0, 0, 0);
	bottomLayout->setSpacing(8);
	presetLabel_ = new QLabel("Presets");
	presetLabel_->setObjectName("transitionPresetLabel");
	bottomLayout->addWidget(presetLabel_);
	railContents->bottom = bottom;
	auto *scroll = new QScrollArea;
	presetScroll_ = scroll;
	scroll->setWidgetResizable(true);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scroll->setFrameShape(QFrame::NoFrame);
	presetArea_ = new QWidget;
	presetArea_->setObjectName("transitionPresets");
	presetArea_->setLayout(new QGridLayout);
	presetArea_->layout()->setContentsMargins(0, 0, 0, 4);
	presetArea_->layout()->setSizeConstraint(QLayout::SetMinimumSize);
	presetArea_->layout()->setSpacing(6);
	scroll->setWidget(presetArea_);
	bottomLayout->addWidget(scroll, 1);

	auto *railScroll = new QScrollArea;
	railScroll->setObjectName("transitionRail");
	railScroll->setWidgetResizable(true);
	railScroll->setFrameShape(QFrame::NoFrame);
	railScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	railScroll->setMinimumWidth(170);
	railScroll->setWidget(railContents);
	railContents->arrange();
	columns_->addWidget(railScroll);
	columns_->setStretchFactor(0, 1);
	columns_->setStretchFactor(1, 0);

	root->addWidget(columns_, 1);
	connect(columns_, &QSplitter::splitterMoved, this, [this] {
		if (config_.locked || rebuilding_) return;
		const auto sizes = columns_->sizes();
		if (sizes.size() != 2) return;
		config_.transitionSettingsWidth = std::clamp(sizes[1], 170, 900);
		fitGrid();
		persist();
	});
	status_ = new QLabel;
	status_->setObjectName("deckStatus");
	status_->setWordWrap(true);
	root->addWidget(status_);
	connect(add_, &QPushButton::clicked, this, [this] { editButton(); });
	connect(edit_, &QPushButton::toggled, this, [this](bool editing) {
		lock_->setChecked(!editing);
	});
	connect(tiles_, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
		if (edit_->isChecked() || !state_.ready)
			return;
		activateButton(Button::fromJson(item->data(DataRole).toJsonObject()).id);
	});
	connect(tiles_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
		if (edit_->isChecked())
			editButton(tiles_->row(item));
	});
	connect(tiles_, &QListWidget::customContextMenuRequested, this, [this](QPoint pos) {
		contextMenu(tiles_->viewport()->mapToGlobal(pos));
	});
	connect(auto_, &QPushButton::clicked, this, [this] { activateButton("@auto"); });
	connect(cut_, &QPushButton::clicked, this, [this] { activateButton("@cut"); });
	connect(transition_, &QComboBox::currentTextChanged, this, [this](const QString &name) {
		if (rebuilding_ || name.isEmpty())
			return;
		QString error;
		if (!backend_.transition(name, duration_->value(), false, error))
			message(error);
		refresh();
	});
	connect(duration_, &QSpinBox::editingFinished, this, [this] {
		QString error;
		if (!backend_.transition(transition_->currentText(), duration_->value(), false, error))
			message(error);
		refresh();
	});
	connect(presetsButton_, &QPushButton::clicked, this, &DeckWidget::editPresets);
	auto *timer = new QTimer(this);
	timer->setInterval(350);
	connect(timer, &QTimer::timeout, this, [this] {
		if (isVisible())
			refresh();
	});
	timer->start();
	auto *lightTimer = new QTimer(this);
	lightTimer->setObjectName("previewLightTimer");
	lightTimer->setInterval(40);
	QElapsedTimer lightClock; lightClock.start();
	connect(lightTimer, &QTimer::timeout, this, [this, lightClock] {
		if (!isVisible() || !state_.studio || state_.preview.isEmpty() || !state_.ready || state_.preview == state_.program) return;
        // Same 1.2 s cadence and phase; shorten only the fades, leaving bright/dim plateaus.
        const qreal phase = .5 + .5 * std::cos(lightClock.elapsed() * 6.283185307179586 / 1200.0);
        const qreal ramp = std::clamp((phase - .35) / .30, 0.0, 1.0);
        const qreal level = .08 + .92 * ramp * ramp * (3 - 2 * ramp);
		for (auto *surface : {static_cast<QWidget *>(tiles_), tiles_->viewport()}) surface->setProperty("previewLight", level);
		for (int i = 0; i < tiles_->count(); ++i)
			if (tiles_->item(i)->data(StateRole).toString() == "PREVI")
				tiles_->viewport()->update(tiles_->visualItemRect(tiles_->item(i)));
	});
	lightTimer->start();
	config_.normalize();
	applyLock();
	rebuild();
	refresh();
}
void DeckWidget::moveButton(const QString &id, int target)
{
	if (config_.locked)
		return;
	int from = -1;
	for (int i = 0; i < config_.buttons.size(); ++i)
		if (config_.buttons[i].id == id)
			from = i;
	if (target < 0)
		target = int(config_.buttons.size()) - 1;
	if (from < 0 || target >= config_.buttons.size() || from == target)
		return;
	config_.buttons.move(from, target);
	persist();
	rebuild();
	refresh();
}
int DeckWidget::freeSlot() const
{
	for (int i = 0; i < config_.buttons.size(); ++i)
		if (config_.buttons[i].type == "empty")
			return i;
	return -1;
}
void DeckWidget::contextMenu(const QPoint &global)
{
	QMenu menu(this);
	menu.setObjectName("deckContextMenu");
	if (config_.locked) {
		auto *configure = menu.addAction("Configurar la botonera…");
		connect(configure, &QAction::triggered, this, [this] { lock_->setChecked(false); });
		menu.exec(global);
		return;
	}
	auto *item = tiles_->itemAt(tiles_->viewport()->mapFromGlobal(global));
	const int index = item ? tiles_->row(item) : -1;
	if (index >= 0) {
		auto *edit = menu.addAction("Editar botó…");
		connect(edit, &QAction::triggered, this, [this, index] { editButton(index); });
		auto *clear = menu.addAction("Buidar casella");
		connect(clear, &QAction::triggered, this, [this, index] {
			auto id = config_.buttons[index].id;
			config_.buttons[index] = Button{};
			config_.buttons[index].id = id;
			config_.buttons[index].type = "empty";
			persist(); rebuild(); refresh();
		});
		menu.addSeparator();
	}
	connect(menu.addAction("Afegir botó…"), &QAction::triggered, add_, &QPushButton::click);
	connect(menu.addAction("Graella i mida…"), &QAction::triggered, this, [this] { configureGrid(); });
	connect(menu.addAction("Presets de transició…"), &QAction::triggered, this, [this] { editPresets(); });
	connect(menu.addAction("Importar botonera…"), &QAction::triggered, this, [this] { importConfig(); });
	connect(menu.addAction("Exportar amb imatges…"), &QAction::triggered, this, [this] { exportConfig(); });
	connect(menu.addAction("Dreceres de teclat…"), &QAction::triggered, this, [this] {
		QMessageBox::information(this, "Dreceres", "A OBS → Configuració → Dreceres, cerca «Botonera».");
	});
	menu.addSeparator();
	connect(menu.addAction("Entrar en realització"), &QAction::triggered, this, [this] { lock_->setChecked(true); });
	menu.exec(global);
}
void DeckWidget::bindControl(QPushButton *button, const QString &id)
{
	auto *control = static_cast<ControlButton *>(button);
	control->setProperty("controlId", id);
	control->setAcceptDrops(true);
	const auto label = button->text();
	control->contextRequested = [this, id, label](const QPoint &global) {
		if (config_.locked) contextMenu(global);
		else editControl(id, label);
	};
	control->imageDropped = [this, id](const QString &image) {
		if (config_.locked || !backend_.snapshot().ready) return;
		config_.controlImages.insert(id, image); persist(); refresh();
		message("Imatge integrada al control. No s’ha executat cap transició.");
	};
}
void DeckWidget::updateControls()
{
	for (auto *button : findChildren<QPushButton *>()) {
		const auto id = button->property("controlId").toString();
		if (id.isEmpty()) continue;
		auto *control = static_cast<ControlButton *>(button);
		control->configurable = !config_.locked && state_.ready;
		control->operational = config_.locked && state_.ready && !state_.busy && button->isEnabled();
		button->setProperty("operational", control->operational);
		control->setArtwork(config_.controlImages.value(id).toString());
		button->setProperty("shortcutLabel", config_.locked ? QString() : shortcutLabel(config_.hotkeys, id));
		button->update();
	}
}
void DeckWidget::activateControl(const QString &id)
{
	const auto snap = backend_.snapshot();
	if (!config_.locked || !snap.ready || snap.busy) return;
	if (id == "@auto" || id == "@cut") {
		if (snap.studio) runTransition(id == "@cut");
		return;
	}
	QString name; int duration = snap.duration;
	if (id.startsWith("@transition:")) name = id.mid(12);
	else if (id.startsWith("@preset:")) {
		for (const auto &preset : config_.presets) if ("@preset:" + preset.id == id) {
			name = preset.transition; duration = preset.duration; break;
		}
	}
	if (name.isEmpty() || !snap.transitions.contains(name)) {
		message("Aquesta transició ja no està disponible."); return;
	}
	QString error;
	if (!backend_.transition(name, duration, false, error)) message(error);
	else message("Transició preparada. Prem AUTO per executar-la.");
	refresh();
}
void DeckWidget::editControl(const QString &id, const QString &label, const QString &image)
{
	if (config_.locked || !backend_.snapshot().ready) return;
	const auto live = backend_.portableConfig(config_);
	const auto previous = live.hotkeys[id].toArray();
	QString artwork = image.isEmpty() ? config_.controlImages.value(id).toString() : image;
	QDialog dialog(this); dialog.setFont(font()); dialog.setObjectName("controlEditor");
	dialog.setWindowTitle("Editar control · " + QString(label).replace('\n', " · "));
	dialog.setMinimumWidth(440);
	auto *root = new QVBoxLayout(&dialog);
	root->addWidget(new QLabel(label));
	QLabel preview; preview.setAlignment(Qt::AlignCenter); preview.setMinimumHeight(100);
	root->addWidget(&preview);
	auto updatePreview = [&] {
		QImage picture; picture.loadFromData(QByteArray::fromBase64(artwork.toLatin1()), "PNG");
		if (picture.isNull()) { preview.setPixmap(QPixmap{}); preview.setText("Sense imatge personalitzada"); }
		else { preview.setText({}); preview.setPixmap(QPixmap::fromImage(picture).scaled(240, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation)); }
	}; updatePreview();
	auto *imageRow = new QHBoxLayout;
	auto *choose = button("Triar imatge…", imageRow);
	auto *clearImage = button("Treure imatge", imageRow);
	root->addLayout(imageRow);
	connect(choose, &QPushButton::clicked, &dialog, [&] {
		const auto path = QFileDialog::getOpenFileName(&dialog, "Imatge del control", {}, "Imatges (*.png *.jpg *.jpeg *.webp *.bmp)");
		if (path.isEmpty()) return;
		const auto encoded = readImage(path);
		if (encoded.isEmpty()) { preview.setText("No s’ha pogut llegir la imatge."); return; }
		artwork = encoded; updatePreview();
	});
	connect(clearImage, &QPushButton::clicked, &dialog, [&] { artwork.clear(); updatePreview(); });
	root->addWidget(new QLabel("Drecera de teclat"));
	auto *shortcutRow = new QHBoxLayout;
	QKeySequenceEdit shortcut(shortcutSequence(previous.isEmpty() ? QJsonObject{} : previous[0].toObject()));
	shortcut.setObjectName("controlShortcut"); shortcut.setMaximumSequenceLength(1);
	shortcut.setClearButtonEnabled(true); shortcut.setFinishingKeyCombinations({});
	if (auto *entry = shortcut.findChild<QLineEdit *>()) entry->setPlaceholderText("Prem una combinació");
	QPushButton lock; lock.setObjectName("controlShortcutLock"); lock.setCheckable(true); lock.setChecked(true);
	lock.setAccessibleName("Bloquejar la drecera");
	auto updateLock = [&] {
		const bool locked = lock.isChecked();
		QPixmap icon(20,20); icon.fill(Qt::transparent); QPainter p(&icon); p.setRenderHint(QPainter::Antialiasing);
		p.setPen(QPen(dialog.palette().color(QPalette::ButtonText), 2));
		p.drawRoundedRect(QRectF(4,9,12,9),2,2); p.drawArc(QRectF(locked ? 6 : 10,2,8,12),0,180*16);
		lock.setIcon(QIcon(icon)); lock.setToolTip(locked ? "Desbloqueja per canviar la drecera" : "Bloqueja la drecera");
		shortcut.setEnabled(!locked); if (!locked) shortcut.setFocus();
	}; connect(&lock, &QPushButton::toggled, &dialog, updateLock); updateLock();
	shortcutRow->addWidget(&shortcut, 1); shortcutRow->addWidget(&lock); root->addLayout(shortcutRow);
	QLabel warning("La combinació pot estar assignada a un altre control d’OBS o del sistema. AUTO executa la transició; CUT fa el tall. Tipus i presets només preparen la transició.");
	warning.setWordWrap(true); root->addWidget(&warning);
	bool changed = false;
	connect(&shortcut, &QKeySequenceEdit::keySequenceChanged, &dialog, [&] {
		changed = true; QString text = "La combinació pot estar assignada a una altra acció d’OBS o del sistema.";
		for (auto it = live.hotkeys.begin(); it != live.hotkeys.end(); ++it) if (it.key() != id)
			for (const auto &binding : it.value().toArray())
				if (!shortcut.keySequence().isEmpty() && shortcutSequence(binding.toObject()) == shortcut.keySequence())
					text = "Atenció: aquesta drecera ja està assignada a un altre control o botó de la botonera.";
		warning.setText(text);
	});
	QDialogButtonBox buttons(QDialogButtonBox::Save | QDialogButtonBox::Cancel); root->addWidget(&buttons);
	buttons.button(QDialogButtonBox::Save)->setText("Desar"); buttons.button(QDialogButtonBox::Cancel)->setText("Cancel·lar");
	connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	connect(&buttons, &QDialogButtonBox::accepted, &dialog, [&] {
		if (changed && !shortcut.keySequence().isEmpty() && shortcutBinding(shortcut.keySequence()).isEmpty()) {
			warning.setText("Configura aquesta tecla especial a les dreceres d’OBS."); return;
		}
		dialog.accept();
	});
	if (dialog.exec() != QDialog::Accepted) return;
	config_ = backend_.portableConfig(config_);
	if (artwork.isEmpty()) config_.controlImages.remove(id); else config_.controlImages.insert(id, artwork);
	if (changed) {
		auto bindings = previous; if (!bindings.isEmpty()) bindings.removeAt(0);
		if (!shortcut.keySequence().isEmpty()) bindings.prepend(shortcutBinding(shortcut.keySequence()));
		config_.hotkeys.insert(id, bindings);
	}
	backend_.replace(config_); refresh();
}
void DeckWidget::configureGrid()
{
	if (config_.locked) return;
	const Config original = config_;
	QDialog dialog(this);
	dialog.setFont(font());
	dialog.setObjectName("gridSettings");
	dialog.setWindowTitle("Graella de la botonera");
	auto *layout = new QVBoxLayout(&dialog);
	layout->addWidget(new QLabel("Columnes × files"));
	auto *dimensions = new QHBoxLayout;
	QSpinBox columns, rows, size;
	columns.setObjectName("gridColumns"); rows.setObjectName("gridRows"); size.setObjectName("tilePixels");
	columns.setRange(1, 200); rows.setRange(1, 200); size.setRange(80, 200); size.setSuffix(" px");
	columns.setValue(config_.columns); rows.setValue(config_.rows); size.setValue(config_.tileSize);
	dimensions->addWidget(&columns); dimensions->addWidget(new QLabel("×")); dimensions->addWidget(&rows);
	layout->addLayout(dimensions);
	QLabel preview;
	preview.setAlignment(Qt::AlignCenter);
	layout->addWidget(&preview);
	auto update = [&] {
		const int c = columns.value(), r = rows.value();
		QPixmap pix(320, 160); pix.fill(Qt::transparent);
		QPainter p(&pix);
		const qreal cell = std::min(300.0 / c, 140.0 / r);
		p.setPen(palette().color(QPalette::Mid)); p.setBrush(palette().color(QPalette::Highlight));
		for (int y = 0; y < r; ++y) for (int x = 0; x < c; ++x)
			p.drawRoundedRect(QRectF(10+x*cell, 10+y*cell, std::max(1.0, cell-3), std::max(1.0, cell-3)), 3, 3);
		preview.setPixmap(pix);
	};
	connect(&columns, &QSpinBox::valueChanged, &dialog, update);
	connect(&rows, &QSpinBox::valueChanged, &dialog, update); update();
	layout->addWidget(new QLabel("Mida de cada casella"));
	auto *sizeRow = new QHBoxLayout;
	QCheckBox autoFit("Adaptar la mida");
	autoFit.setObjectName("autoFit");
	autoFit.setChecked(config_.autoFit);
	autoFit.setToolTip("Ajusta automàticament per amplada o alçada perquè càpiga tota la graella, amb botons quadrats.");
	size.setEnabled(!autoFit.isChecked());
	connect(&autoFit, &QCheckBox::toggled, &size, [&size](bool enabled) { size.setEnabled(!enabled); });
	sizeRow->addWidget(&size); sizeRow->addWidget(&autoFit);
	layout->addLayout(sizeRow);
	layout->addWidget(new QLabel("Text dels botons de la graella"));
	QCheckBox showLabels("Mostrar les etiquetes"), showStateText("Mostrar PREVI, EN DIRECTE i els altres estats");
	showLabels.setObjectName("showLabels"); showStateText.setObjectName("showStateText");
	showLabels.setChecked(config_.showLabels); showStateText.setChecked(config_.showStateText);
	layout->addWidget(&showLabels); layout->addWidget(&showStateText);
	auto *numberHint = new QLabel("El número sempre es mostra. En caselles molt petites té prioritat sobre els textos.");
	numberHint->setWordWrap(true); layout->addWidget(numberHint);
	QLabel error; error.setWordWrap(true); layout->addWidget(&error);
	auto previewGrid = [&] {
		const int count = columns.value() * rows.value();
		if (count > 200) { error.setText("Màxim 200 caselles."); return; }
		for (int i = count; i < original.buttons.size(); ++i)
			if (original.buttons[i].type != "empty") {
				error.setText("Hi ha botons ocupats fora d’aquesta graella."); return;
			}
		error.clear();
		config_ = original;
		config_.columns = columns.value(); config_.rows = rows.value();
		config_.tileSize = size.value(); config_.autoFit = autoFit.isChecked();
		config_.showLabels = showLabels.isChecked(); config_.showStateText = showStateText.isChecked();
		if (config_.buttons.size() > count) config_.buttons.resize(count);
		config_.normalize(); rebuild(); refresh();
	};
	connect(&columns, &QSpinBox::valueChanged, &dialog, previewGrid);
	connect(&rows, &QSpinBox::valueChanged, &dialog, previewGrid);
	connect(&size, &QSpinBox::valueChanged, &dialog, previewGrid);
	connect(&autoFit, &QCheckBox::toggled, &dialog, previewGrid);
	connect(&showLabels, &QCheckBox::toggled, &dialog, previewGrid);
	connect(&showStateText, &QCheckBox::toggled, &dialog, previewGrid);

	QDialogButtonBox buttons(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
	layout->addWidget(&buttons);
	connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	connect(&buttons, &QDialogButtonBox::accepted, &dialog, [&] {
		const int count = columns.value() * rows.value();
		if (count > 200) { error.setText("Màxim 200 caselles."); return; }
		for (int i=count; i<original.buttons.size(); ++i)
			if (original.buttons[i].type != "empty") {
				error.setText("Reordena o buida els botons que quedarien fora de la graella."); return;
			}
		config_.columns=columns.value(); config_.rows=rows.value(); config_.tileSize=size.value();
		config_.autoFit = autoFit.isChecked();
		config_.showLabels = showLabels.isChecked(); config_.showStateText = showStateText.isChecked();
		if (config_.buttons.size() > count) config_.buttons.resize(count);
		config_.normalize(); persist(); rebuild(); refresh(); dialog.accept();
	});
	if (dialog.exec() != QDialog::Accepted) {
		const bool locked = config_.locked;
		config_ = original; config_.locked = locked; rebuild(); refresh();
	}
}
void DeckWidget::outputStarting()
{
	// A start is a one-shot mode change, not a restriction for the whole broadcast.
	for (auto *menu : findChildren<QMenu *>())
		if (menu->isVisible()) menu->close();
	for (auto *dialog : findChildren<QDialog *>())
		if (dialog->isVisible() && dialog->isModal()) dialog->reject();
	config_.locked = true;
	applyLock(); refresh();
	message("Inici de transmissió o enregistrament · Mode realització");
	// Avoid saving re-entrantly while OBS is dispatching its output-start event.
	QTimer::singleShot(0, this, [this] { persist(); });
}
void DeckWidget::applyLock()
{
	auto *list = static_cast<ImageDropList *>(tiles_);
	list->locked = config_.locked;
	for (int i = 1; i < columns_->count(); ++i) {
		auto *handle = columns_->handle(i);
		handle->setEnabled(!config_.locked);
		handle->setCursor(config_.locked ? Qt::ArrowCursor : Qt::SplitHCursor);
		handle->setToolTip(config_.locked ? QString() : "Arrossega per canviar l’amplada de la botonera i les columnes.");
		handle->setStyleSheet(config_.locked ? "background: transparent;" : "background: rgba(130,145,165,70); border-radius:3px;");
	}
	// Persisted lock is the single source of truth for operating mode.
	QSignalBlocker editSignal(edit_), lockSignal(lock_);
	edit_->setChecked(!config_.locked);
	lock_->setChecked(config_.locked);
	edit_->setEnabled(true);
	list->reorderEnabled = !config_.locked;
	tiles_->setMovement(config_.locked ? QListView::Static : QListView::Snap);
	tiles_->setDragDropMode(config_.locked ? QAbstractItemView::DropOnly : QAbstractItemView::DragDrop);
	tiles_->setDragEnabled(!config_.locked);
	tiles_->setAcceptDrops(true);
	tiles_->viewport()->setAcceptDrops(true);
	tiles_->setProperty("configurationMode", !config_.locked);
	tiles_->viewport()->setProperty("configurationMode", !config_.locked);
	tiles_->viewport()->update();
	tiles_->setToolTip(config_.locked ? "Realització · Clic dret per tornar a configurar" :
		"Configuració · Arrossega per ordenar · Clic dret per editar un botó");
	empty_->setVisible(!config_.locked && std::none_of(config_.buttons.begin(), config_.buttons.end(),
		[](const Button &b) { return b.type != "empty"; }));
	status_->setText(config_.locked ? "Realització · Clic dret per configurar" :
		"Configuració · Arrossega per ordenar · Clic dret per editar");
}
void DeckWidget::assignFromSource(QObject *source, int index)
{
	if (config_.locked || !backend_.snapshot().ready)
		return;
	auto *view = qobject_cast<QAbstractItemView *>(source);
	if (!view || view->objectName() != "scenes")
		return;
	const QString name = view->currentIndex().data(Qt::DisplayRole).toString();
	const auto snap = backend_.snapshot();
	Button action;
	bool found = false;
	for (QObject *ancestor = view->parent(); ancestor && !found; ancestor = ancestor->parent())
		for (const auto &k : snap.keyers)
			if (ancestor->objectName() == k.name && k.scenes.contains(name)) {
				action.type = "dsk";
				action.channel = k.name;
				action.name = name;
				found = true;
				break;
			}
	if (!found)
		for (const auto &scene : snap.scenes)
			if (scene.name == name) {
				action.type = "scene";
				action.target = scene.id;
				action.name = name;
				found = true;
				break;
			}
	if (!found) {
		message("No s’ha pogut identificar l’escena arrossegada. Utilitza + Afegir.");
		return;
	}
	if (index < 0)
		index = freeSlot();
	if (index < 0) {
		message("No hi ha caselles lliures. Augmenta el nombre de caselles.");
		return;
	}
	const auto previous = config_.buttons[index];
	action.id = previous.id;
	action.image = previous.image;
	action.color = previous.color;
	action.crop = previous.crop;
	action.label = name;
	config_.buttons[index] = action;
	persist();
	rebuild();
	refresh();
	message("Botó afegit des del panell d’origen.");
}
void DeckWidget::message(const QString &s)
{
	status_->setText(s);
}
void DeckWidget::activateButton(const QString &id)
{
	for (auto *dialog : findChildren<QDialog *>())
		if (dialog->isVisible() && dialog->isModal()) {
			message("Drecera bloquejada: hi ha una finestra de configuració oberta.");
			return;
		}
	if (edit_->isChecked()) {
		message("Drecera bloquejada: la botonera està en mode edició.");
		return;
	}
	if (!backend_.snapshot().ready) {
		message("La botonera encara no està preparada.");
		return;
	}
	if (id.startsWith("@")) { activateControl(id); return; }
	const auto it = std::find_if(config_.buttons.begin(), config_.buttons.end(),
				     [&](const Button &b) { return b.id == id; });
	if (it == config_.buttons.end())
		return;
	if (it->type == "empty") {
		if (!config_.locked)
			editButton(int(it - config_.buttons.begin()));
		return;
	}
	QString error;
	if (!backend_.execute(*it, error))
		message(error);
	else
		message("Acció executada: " + it->label);
	refresh();
}
void DeckWidget::persist()
{
	backend_.save(config_);
}
void DeckWidget::restoreColumnWidths()
{
	const int settings = config_.transitionSettingsWidth;
	columns_->setSizes({std::max(100, columns_->width() - settings - columns_->handleWidth()), settings});
	fitGrid();
}
void DeckWidget::load(const Config &c)
{
	config_ = c;
	config_.normalize();
	QTimer::singleShot(0, this, [this] { restoreColumnWidths(); });
	QSignalBlocker blockLock(lock_);
	lock_->setChecked(config_.locked);
	applyLock();
	rebuild();
	refresh();
}
bool DeckWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == gridScroll_->viewport() && event->type() == QEvent::Resize)
		fitGrid();
	return QWidget::eventFilter(object, event);
}
void DeckWidget::fitGrid()
{
	const int frame = 2 * tiles_->frameWidth();
	int side = config_.tileSize;
	gridScroll_->setHorizontalScrollBarPolicy(config_.autoFit ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
	gridScroll_->setVerticalScrollBarPolicy(config_.autoFit ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
	if (config_.autoFit) {
		const QSize available = gridScroll_->viewport()->size();
		side = std::max(1, std::min((available.width() - frame - 1) / config_.columns,
					  (available.height() - frame) / config_.rows));
	}
	const QSize size(side, side);
	tiles_->setGridSize(size);
	tiles_->setFixedSize(config_.columns * side + frame + 1, config_.rows * side + frame);
	for (int i = 0; i < tiles_->count(); ++i)
		tiles_->item(i)->setSizeHint(size);
	tiles_->doItemsLayout();
}
void DeckWidget::rebuild()
{
	rebuilding_ = true;
	tiles_->clear();
	for (const auto &b : config_.buttons) {
		auto *i = new QListWidgetItem(
			b.type == "empty" ? "Casella " + QString::number(tiles_->count() + 1) : b.label, tiles_);

		i->setData(DataRole, b.json());
		i->setToolTip(b.label + "\n" + b.name + (b.channel.isEmpty() ? "" : " · " + b.channel));
		i->setData(Qt::AccessibleTextRole, b.label);
	}
	empty_->setVisible(!config_.locked && std::none_of(config_.buttons.begin(), config_.buttons.end(),
					[](const Button &b) { return b.type != "empty"; }));
	fitGrid();
	auto *layout = static_cast<QGridLayout *>(presetArea_->layout());
	for (int row = 0; row < layout->rowCount(); ++row) {
		layout->setRowStretch(row, 0);
		layout->setRowMinimumHeight(row, 0);
	}
	while (auto *item = layout->takeAt(0)) {
		delete item->widget();
		delete item;
	}
	const auto transitionKinds = backend_.snapshot().transitionKinds;
	int presetIndex = 0;
	for (const auto &p : config_.presets) {
		const auto code = compactTransitionCode(p.transition, transitionKinds.value(p.transition));
		const bool cut = isCutTransition(p.transition, transitionKinds.value(p.transition));
		auto *b = new ControlButton(cut ? code : code + QString::number(p.duration));
		b->setObjectName("transitionPreset");
		layout->addWidget(b, presetIndex / 2, presetIndex % 2);
		++presetIndex;
		bindControl(b, "@preset:" + p.id);
		b->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
		b->setMinimumWidth(0);
		const auto description = p.name + " · " + p.transition + (cut ? QString() : " · " + QString::number(p.duration) + " ms");
		b->setToolTip(description);
		b->setAccessibleName(description);
		// Resolve the OBS stylesheet before sizing rows; polishing later can otherwise make buttons overlap.
		b->ensurePolished();
		b->setFixedHeight(38);
		layout->setRowMinimumHeight((presetIndex - 1) / 2, 38);

		b->setProperty("transitionName", p.transition);
		connect(b, &QPushButton::clicked, this, [this, p] { activateButton("@preset:" + p.id); });
	}
	layout->setColumnStretch(0, 1);
	layout->setColumnStretch(1, 1);
	layout->setAlignment(Qt::AlignBottom);
	rebuilding_ = false;
}
void DeckWidget::refresh()
{
	for (auto *surface : {static_cast<QWidget *>(tiles_), tiles_->viewport()}) {
		surface->setProperty("hideLabels", !config_.showLabels);
		surface->setProperty("hideStateText", !config_.showStateText);
	}
	for (int i = 0; i < tiles_->count(); ++i) {
		auto *item = tiles_->item(i);
		const auto id = Button::fromJson(item->data(DataRole).toJsonObject()).id;
		item->setData(Qt::UserRole + 4, config_.locked ? QString() : shortcutLabel(config_.hotkeys, id));
	}
	state_ = backend_.snapshot();
	auto sceneName = [this](const QString &id) {
		for (const auto &s : state_.scenes)
			if (s.id == id)
				return s.name;
		return QString("—");
	};
	mode_->setText(QString(config_.locked ? "REALITZACIÓ · " : "CONFIGURACIÓ · ") + (!state_.ready ? (state_.blockedReason.isEmpty() ? "Canviant col·lecció…" : state_.blockedReason)
		       : state_.studio ? "MODE ESTUDI · PREVI: " + sceneName(state_.preview) +
						 "  |  EN DIRECTE: " + sceneName(state_.program)
				       : "MODE DIRECTE · Els botons d’escena actuen sobre programa: " +
						 sceneName(state_.program)));
	for (int i = 0; i < tiles_->count(); ++i) {
		auto *item = tiles_->item(i);
		const auto b = Button::fromJson(item->data(DataRole).toJsonObject());
		QString label;
		if (!state_.ready)
			label = "ESPERA";
		else if (b.type == "scene") {
			auto it = std::find_if(state_.scenes.begin(), state_.scenes.end(), [&](const Scene &s) {
				return b.target.isEmpty() ? s.name == b.name : s.id == b.target;
			});
			if (it == state_.scenes.end())
				label = "FALTA ESCENA";
			else if (it->id == state_.program && state_.studio && it->id == state_.preview)
				label = "DIRECTE + PREVI";
			else if (it->id == state_.program)
				label = "EN DIRECTE";
			else if (state_.studio && it->id == state_.preview)
				label = "PREVI";
		} else if (b.type != "empty") {
			auto k = std::find_if(state_.keyers.begin(), state_.keyers.end(),
					      [&](const Keyer &v) { return v.name == b.channel; });
			if (k == state_.keyers.end())
				label = "FALTA DSK";
			else if (k->tied)
				label = "DSK TIE";
			else if (b.type == "dsk" && !k->scenes.contains(b.name))
				label = "FALTA ELEMENT";
			else if ((b.type == "dsk-clear" && k->current.isEmpty()) ||
				 (b.type == "dsk" && k->current == b.name))
				label = "ACTIU";
			else
				label = b.type == "dsk" ? "INACTIU" : "DSK · NETEJAR";
		}
		if (b.type == "dsk")
			item->setToolTip(b.label + " · " + (label == "ACTIU" ? "Clic per amagar" : "Clic per mostrar") +
				" amb l’efecte configurat al Downstream Keyer (" + b.channel + ")");
		if (item->data(StateRole).toString() != label)
			item->setData(StateRole, label);
	}
	{
		QSignalBlocker b(transition_);
		if (state_.transitions != QStringList([&] {
			    QStringList a;
			    for (int i = 0; i < transition_->count(); ++i)
				    a << transition_->itemText(i);
			    return a;
		    }())) {
			transition_->clear();
			transition_->addItems(state_.transitions);
		}
		transition_->setCurrentText(state_.transition);
	}
	auto orderedTransitions = state_.transitions;
	auto rank = [this](const QString &name) {
		const auto code = compactTransitionCode(name, state_.transitionKinds.value(name));
		if (isCutTransition(name, state_.transitionKinds.value(name))) return 0;
		if (code == "F") return 1;
		if (code == "M") return 2;
		return 3;
	};
	std::stable_sort(orderedTransitions.begin(), orderedTransitions.end(), [&](const QString &a, const QString &b) { return rank(a) < rank(b); });
	if (renderedTransitions_ != orderedTransitions) {
		renderedTransitions_ = orderedTransitions;
		auto *layout = static_cast<QGridLayout *>(transitionChoices_->layout());
		while (auto *item = layout->takeAt(0)) {
			delete item->widget();
			delete item;
		}
		for (int i = 0; i < orderedTransitions.size(); ++i) {
			const auto name = orderedTransitions[i];
			auto *b = new ScrubButton(name);
			b->setAccessibleName(name);
			b->setObjectName("transitionChoice");
			b->setProperty("transitionName", name);
			bindControl(b, "@transition:" + name);
			b->setCheckable(true);
			b->setText(compactTransitionCode(name, state_.transitionKinds.value(name)));
			b->ensurePolished();
			b->setFixedHeight(36);
			b->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
			b->setToolTip(name + " · Arrossega amunt o avall per ajustar la durada");
			layout->addWidget(b, i / 4, i % 4);
			layout->setColumnStretch(i % 4, 1);
			b->ensurePolished();
			layout->setRowMinimumHeight(i / 4, std::max(b->minimumHeight(), b->sizeHint().height()));
			b->duration = [this] {
				return duration_->value();
			};
			b->scrub = [this, name](int ms) {
				scrubTransition(name, ms);
			};
			connect(b, &QPushButton::clicked, this, [this, name] { activateButton("@transition:" + name); });
		}
		layout->setSizeConstraint(QLayout::SetMinimumSize);
		layout->activate();
		transitionChoices_->setFixedHeight(layout->minimumSize().height());
	}
	if (!duration_->hasFocus()) {
		QSignalBlocker b(duration_);
		duration_->setValue(state_.duration);
	}
	const bool canRun = state_.ready && !state_.busy && !edit_->isChecked();
	auto_->setEnabled(state_.ready && (!config_.locked || (canRun && state_.studio)));
	cut_->setEnabled(state_.ready && (!config_.locked || (canRun && state_.studio)));
	transition_->setEnabled(canRun);
	const bool cutSelected = isCutTransition(state_.transition, state_.transitionKinds.value(state_.transition));
	duration_->setEnabled(canRun && !cutSelected);
	QVector<int> times = {100, 200, 400, 600, 800, 1000, 1200};
	if (!times.contains(state_.duration)) times.append(state_.duration);
	std::sort(times.begin(), times.end());
	const auto timeButtons = durationChoices_->findChildren<QPushButton *>("durationChoice");
	for (int i = 0; i < timeButtons.size(); ++i) {
		auto *choice = timeButtons[i];
		choice->setVisible(i < times.size());
		choice->setChecked(false);
		choice->setEnabled(canRun && !cutSelected && i < times.size());
		if (i >= times.size()) continue;
		choice->setText(QString::number(times[i]));
		choice->setProperty("milliseconds", times[i]);
		choice->setAccessibleName(QString::number(times[i]) + " ms");
		choice->setToolTip(cutSelected ? "El tall és instantani" : QString::number(times[i]) + " ms");
		choice->setChecked(!cutSelected && times[i] == state_.duration);
		choice->setEnabled(canRun && !cutSelected);
	}
	for (auto *b : transitionChoices_->findChildren<QPushButton *>()) {
		b->setEnabled(state_.ready && (!config_.locked || canRun));
		b->setChecked(b->property("transitionName").toString() == state_.transition);
	}

	add_->setEnabled(state_.ready && !config_.locked);
	presetsButton_->setEnabled(state_.ready && !config_.locked);
	for (auto *b : presetArea_->findChildren<QPushButton *>()) {
		b->setEnabled(state_.ready && (!config_.locked || (canRun && state_.transitions.contains(b->property("transitionName").toString()))));
		b->setCheckable(true);
		b->setChecked(b->property("transitionName").toString() == state_.transition &&
			      std::any_of(config_.presets.begin(), config_.presets.end(), [&](const Preset &p) {
				      return b->property("controlId").toString() == "@preset:" + p.id && (cutSelected || p.duration == state_.duration);
			      }));
	}
	updateControls();
}
void DeckWidget::scrubTransition(const QString &name, int ms)
{
	if (edit_->isChecked())
		return;
	QString error;
	if (!backend_.transition(name, ms, false, error))
		message(error);
	else {
		QSignalBlocker block(duration_);
		duration_->setValue(ms);
		message(name + " · " + QString::number(ms) + " ms");
	}
	refresh();
}
void DeckWidget::runTransition(bool cut)
{
	QString error;
	if (!backend_.transition(cut ? "@cut" : transition_->currentText(), duration_->value(), true, error))
		message(error);
	else
		message(cut ? "Tall executat." : "Transició executada.");
	refresh();
}

void DeckWidget::editButton(int index, const QString &image)
{
	const auto snap = backend_.snapshot();
	if (!snap.ready || config_.locked)
		return;
	if (index < 0)
		index = freeSlot();
	if (index < 0) {
		message("No hi ha caselles lliures. Augmenta el nombre de caselles.");
		return;
	}
	Button value = config_.buttons[index];
	if (value.type == "empty")
		value.type = "scene";
	if (!image.isEmpty())
		value.image = image;
	QDialog dialog(this);
	dialog.setFont(font());
	dialog.setObjectName("buttonEditor");
	dialog.setWindowTitle(index >= 0 ? "Editar botó" : "Afegir a la botonera");
	dialog.setMinimumWidth(440);
	auto *root = new QVBoxLayout(&dialog);
	auto *form = new QFormLayout;
	root->addLayout(form);
	QComboBox kind;
	kind.addItem("Escena → previ en mode estudi", "scene");
	kind.addItem("Composició DSK → activar / desactivar", "dsk");
	kind.addItem("Netejar canal DSK → sortida en directe", "dsk-clear");
	form->addRow("Acció", &kind);
	QComboBox channel, target;
	channel.setObjectName("buttonChannel");
	target.setObjectName("buttonTarget");
	QLineEdit label(value.label);
	label.setObjectName("buttonLabel");
	label.setMaxLength(120);
	form->addRow("Canal DSK", &channel);
	form->addRow("Element", &target);
	form->addRow("Etiqueta", &label);
	const auto liveHotkeys = backend_.portableConfig(config_).hotkeys;
	const auto previousBindings = liveHotkeys[value.id].toArray();
	QKeySequenceEdit shortcut(shortcutSequence((previousBindings.isEmpty() ? QJsonObject{} : previousBindings.at(0).toObject())));
	shortcut.setObjectName("buttonShortcut");
	shortcut.setMaximumSequenceLength(1);
	shortcut.setClearButtonEnabled(true);
	if (auto *entry = shortcut.findChild<QLineEdit *>())
		entry->setPlaceholderText("Prem una combinació");
	shortcut.setFinishingKeyCombinations({});
	shortcut.setEnabled(false);
	bool shortcutChanged = false;
	QPushButton shortcutLock;
	shortcutLock.setObjectName("shortcutLock");
	shortcutLock.setCheckable(true);
	shortcutLock.setChecked(true);
	shortcutLock.setAccessibleName("Bloquejar la drecera");
	auto updateLock = [&] {
		const bool locked = shortcutLock.isChecked();
		QPixmap pix(20, 20); pix.fill(Qt::transparent);
		QPainter painter(&pix); painter.setRenderHint(QPainter::Antialiasing);
		painter.setPen(QPen(dialog.palette().color(QPalette::ButtonText), 2));
		painter.drawRoundedRect(QRectF(4, 9, 12, 9), 2, 2);
		painter.drawArc(QRectF(locked ? 6 : 10, 2, 8, 12), 0, 180 * 16);
		shortcutLock.setIcon(QIcon(pix));
		shortcutLock.setToolTip(locked ? "Desbloqueja per canviar la drecera" : "Bloqueja la drecera");
		shortcut.setEnabled(!locked);
		if (!locked) shortcut.setFocus();
	};
	connect(&shortcutLock, &QPushButton::toggled, &dialog, updateLock); updateLock();
	auto *shortcutRow = new QHBoxLayout;
	shortcutRow->addWidget(&shortcut, 1); shortcutRow->addWidget(&shortcutLock);
	form->addRow(previousBindings.size() > 1 ? "Drecera principal" : "Drecera de teclat", shortcutRow);
	QLabel shortcutWarning("Desbloqueja el candau i prem la combinació. Pot estar assignada a una altra acció d’OBS o del sistema.");
	shortcutWarning.setObjectName("shortcutWarning");
	shortcutWarning.setWordWrap(true);
	root->addWidget(&shortcutWarning);
	connect(&shortcut, &QKeySequenceEdit::keySequenceChanged, &dialog, [&] {
		shortcutChanged = true;
		const auto sequence = shortcut.keySequence();
		QString warning = "Aquesta combinació pot estar assignada a una altra acció d’OBS o del sistema.";
		for (auto it = liveHotkeys.begin(); it != liveHotkeys.end(); ++it)
			if (it.key() != value.id)
				for (const auto &binding : it.value().toArray())
					if (!sequence.isEmpty() && shortcutSequence(binding.toObject()) == sequence)
						warning = "Atenció: aquesta drecera ja està assignada a un altre botó de la botonera.";
		shortcutWarning.setText(warning);
	});
	if (!previousBindings.isEmpty() && shortcut.keySequence().isEmpty())
		shortcutWarning.setText("Drecera especial configurada a OBS. Es conserva si no la canvies aquí.");
	QLabel warning("DSK: un clic mostra la composició i un altre l’amaga, amb l’efecte d’entrada o sortida configurat al Downstream Keyer. Actua sobre la sortida en directe. Amb TIE actiu queda bloquejat.");
	warning.setWordWrap(true);
	warning.setWordWrap(true);
	root->addWidget(&warning);
	auto fillTargets = [&] {
		target.clear();
		const QString type = kind.currentData().toString();
		channel.setEnabled(type.startsWith("dsk"));
		warning.setVisible(type.startsWith("dsk"));
		target.setEnabled(type != "dsk-clear");
		if (type == "scene")
			for (const auto &s : snap.scenes)
				target.addItem(s.name, s.id);
		else
			for (const auto &k : snap.keyers)
				if (k.name == channel.currentText())
					for (const auto &s : k.scenes)
						target.addItem(s, s);
	};
	for (const auto &k : snap.keyers)
		channel.addItem(k.name);
	if (!value.channel.isEmpty() && channel.findText(value.channel) < 0)
		channel.addItem(value.channel);
	channel.setCurrentText(value.channel);
	kind.setCurrentIndex(std::max(0, kind.findData(value.type)));
	fillTargets();
	if (!value.name.isEmpty()) {
		int n = value.type == "scene" ? target.findData(value.target) : target.findText(value.name);
		if (n < 0) {
			target.addItem(value.name + " (no disponible)",
				       value.type == "scene" ? value.target : value.name);
			n = target.count() - 1;
		}
		target.setCurrentIndex(n);
	}
	connect(&kind, &QComboBox::currentIndexChanged, &dialog, [&] { fillTargets(); });
	connect(&channel, &QComboBox::currentTextChanged, &dialog, [&] { fillTargets(); });
	auto *imageRow = new QHBoxLayout;
	auto *choose = button("Triar imatge…", imageRow);
	auto *clear = button("Treure imatge", imageRow);
	auto *color = button("Color…", imageRow);
	root->addLayout(imageRow);
	QLabel imageStatus(value.image.isEmpty() ? "Sense imatge" : "Imatge personalitzada inclosa");
	root->addWidget(&imageStatus);
	QCheckBox crop("Omplir el botó retallant la imatge");
	crop.setChecked(value.crop);
	root->addWidget(&crop);
	connect(choose, &QPushButton::clicked, &dialog, [&] {
		const QString path = QFileDialog::getOpenFileName(&dialog, "Imatge del botó", {},
								  "Imatges (*.png *.jpg *.jpeg *.webp *.bmp)");
		if (path.isEmpty())
			return;
		const auto encoded = readImage(path);
		if (encoded.isEmpty()) {
			imageStatus.setText("Imatge no compatible o massa gran.");
			return;
		}
		value.image = encoded;
		imageStatus.setText("Imatge inclosa · " + QFileInfo(path).fileName());
	});
	connect(clear, &QPushButton::clicked, &dialog, [&] {
		value.image.clear();
		imageStatus.setText("Sense imatge");
	});
	connect(color, &QPushButton::clicked, &dialog, [&] {
		auto c = QColorDialog::getColor(QColor(value.color), &dialog, "Color del botó");
		if (c.isValid())
			value.color = c.name();
	});
	QDialogButtonBox buttons(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
	root->addWidget(&buttons);
	connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	connect(&buttons, &QDialogButtonBox::accepted, &dialog, [&] {
		if (shortcutChanged && !shortcut.keySequence().isEmpty() && shortcutBinding(shortcut.keySequence()).isEmpty()) {
			shortcutWarning.setText("Aquesta tecla no es pot assignar aquí. Configura-la a les dreceres d’OBS.");
			return;
		}
		const QString type = kind.currentData().toString();
		if ((type == "scene" || type == "dsk") && target.currentIndex() < 0) {
			imageStatus.setText("Selecciona un element disponible.");
			return;
		}
		if (type.startsWith("dsk") && channel.currentText().isEmpty()) {
			imageStatus.setText("No hi ha canals DSK disponibles. Comprova el complement.");
			return;
		}
		value.type = type;
		value.target = type == "scene" ? target.currentData().toString() : QString{};
		// Preserve the real stored name for an unavailable selection.
		if (target.currentText() != value.name + " (no disponible)")
			value.name = type == "dsk-clear" ? QString{} : target.currentText();
		value.channel = type == "scene" ? QString{} : channel.currentText();
		value.crop = crop.isChecked();
		value.label = label.text().trimmed();
		if (value.label.isEmpty())
			value.label = type == "dsk-clear" ? "Netejar " + value.channel : value.name;
		if (value.id.isEmpty())
			value.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
		dialog.accept();
	});
	if (dialog.exec() != QDialog::Accepted)
		return;
	config_.buttons[index] = value;
	if (shortcutChanged) {
		config_ = backend_.portableConfig(config_);
		auto bindings = previousBindings;
		if (!bindings.isEmpty()) bindings.removeAt(0);
		if (!shortcut.keySequence().isEmpty()) bindings.prepend(shortcutBinding(shortcut.keySequence()));
		config_.hotkeys.insert(value.id, bindings);
		backend_.replace(config_);
	} else persist();
	rebuild();
	refresh();
}

void DeckWidget::editPresets()
{
	if (config_.locked)
		return;
	auto presets = config_.presets;
	const auto snap = backend_.snapshot();
	QDialog dialog(this);
	dialog.setFont(font());
	dialog.setWindowTitle("Presets de transició");
	dialog.setMinimumWidth(420);
	auto *root = new QVBoxLayout(&dialog);
	QListWidget list;
	root->addWidget(&list);
	auto *form = new QFormLayout;
	root->addLayout(form);
	QLineEdit name;
	QComboBox type;
	type.addItems(snap.transitions);
	QSpinBox duration;
	duration.setRange(50, 20000);
	duration.setValue(300);
	duration.setSuffix(" ms");
	form->addRow("Nom", &name);
	form->addRow("Transició", &type);
	form->addRow("Durada", &duration);
	auto *actions = new QHBoxLayout;
	auto *add = button("Afegir", actions);
	auto *update = button("Actualitzar", actions);
	auto *remove = button("Treure", actions);
	root->addLayout(actions);
	auto populate = [&] {
		list.clear();
		for (const auto &p : presets)
			list.addItem(p.name + " · " + p.transition + " · " + QString::number(p.duration) + " ms");
	};
	populate();
	connect(&list, &QListWidget::currentRowChanged, &dialog, [&](int i) {
		if (i >= 0 && i < presets.size()) {
			name.setText(presets[i].name);
			type.setCurrentText(presets[i].transition);
			duration.setValue(presets[i].duration);
		}
	});
	auto current = [&] {
		return Preset{name.text().trimmed(), type.currentText(), duration.value()};
	};
	connect(add, &QPushButton::clicked, &dialog, [&] {
		if (!name.text().trimmed().isEmpty() && !type.currentText().isEmpty() && presets.size() < 32) {
			presets.push_back(current());
			populate();
		}
	});
	connect(update, &QPushButton::clicked, &dialog, [&] {
		int i = list.currentRow();
		if (i >= 0 && !name.text().trimmed().isEmpty() && !type.currentText().isEmpty()) {
			const auto id = presets[i].id;
			presets[i] = current(); presets[i].id = id;
			populate();
		}
	});
	connect(remove, &QPushButton::clicked, &dialog, [&] {
		int i = list.currentRow();
		if (i >= 0) {
			presets.removeAt(i);
			populate();
		}
	});
	QDialogButtonBox buttons(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
	root->addWidget(&buttons);
	connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	if (dialog.exec() == QDialog::Accepted) {
		config_.presets = presets;
		config_.normalize();
		persist();
		rebuild();
		refresh();
	}
}
void DeckWidget::importConfig()
{
	if (config_.locked) {
		message("Desbloqueja la botonera abans d’importar.");
		return;
	}
	const QString path = QFileDialog::getOpenFileName(this, "Importar botonera", {}, "Botonera (*.json)");
	if (path.isEmpty())
		return;
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly) || file.size() > 32 * 1024 * 1024) {
		message("No es pot llegir el fitxer (màxim 32 MB).");
		return;
	}
	QJsonParseError parse;
	auto doc = QJsonDocument::fromJson(file.readAll(), &parse);
	Config config;
	QString error;
	if (parse.error != QJsonParseError::NoError || !doc.isObject()) {
		message("JSON incorrecte.");
		return;
	}
	if (!Config::parse(doc.object(), config, error)) {
		message(error);
		return;
	}
	if (QMessageBox::question(
		    this, "Importar botonera",
		    "Substituir els botons i presets d’aquesta col·lecció? Les escenes d’OBS no es modifiquen.") !=
	    QMessageBox::Yes)
		return;
	backend_.replace(config);
	load(config);
	message("Botonera importada. Els elements no disponibles queden indicats als botons.");
}
void DeckWidget::exportConfig()
{
	const QString path =
		QFileDialog::getSaveFileName(this, "Exportar amb les imatges", "botonera.json", "Botonera (*.json)");
	if (path.isEmpty())
		return;
	QSaveFile file(path);
	const auto bytes = QJsonDocument(backend_.portableConfig(config_).json()).toJson();
	if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
		message("No s’ha pogut desar el fitxer.");
	else
		message("Botonera exportada amb les imatges.");
}
} // namespace deck
