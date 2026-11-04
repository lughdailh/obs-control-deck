#pragma once
#include "deck-model.hpp"
#include <QWidget>
class QListWidget;
class QLabel;
class QComboBox;
class QSpinBox;
class QPushButton;
class QTimer;
class QScrollArea;
class QSplitter;

namespace deck {
class DeckWidget : public QWidget {
public:
	explicit DeckWidget(Backend &, QWidget *parent = nullptr);
	void load(const Config &);
	const Config &config() const { return config_; }
	void refresh();
	void outputStarting();
	void activateButton(const QString &id);
	void assignFromSource(QObject *, int);
	void moveButton(const QString &, int);

protected:
	bool eventFilter(QObject *, QEvent *) override;

private:
	Backend &backend_;
	Config config_;
	Snapshot state_;
	QListWidget *tiles_;
	QScrollArea *gridScroll_;
	QSplitter *columns_;
	void restoreColumnWidths();
	void fitGrid();
	QLabel *mode_, *status_, *empty_;
	QComboBox *transition_;
	QSpinBox *duration_;
	QPushButton *edit_, *auto_, *cut_, *add_, *presetsButton_, *lock_;
	QWidget *presetArea_, *transitionChoices_, *durationChoices_, *presetScroll_, *presetLabel_;
	void configureGrid();
	void bindControl(QPushButton *, const QString &);
	void updateControls();
	void editControl(const QString &, const QString &, const QString &image = {});
	void activateControl(const QString &);
	void contextMenu(const QPoint &);
	QStringList renderedTransitions_;
	bool rebuilding_ = false;
	void rebuild();
	void persist();
	void editButton(int index = -1, const QString &image = {});
	int freeSlot() const;
	void applyLock();
	void editPresets();
	void runTransition(bool cut);
	void scrubTransition(const QString &, int);
	void importConfig();
	void exportConfig();
	void message(const QString &);
};
} // namespace deck
