#include "dialogs.h"
#include "dialogs.moc"

#include <qpainter.h>
#include <qpixmap.h>
#include <qfont.h>
#include <qvgroupbox.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qgrid.h>

#include <kapplication.h>
#include <klocale.h>
#include <kconfig.h>

#include "bitmaps/smile"
#include "bitmaps/smile_happy"
#include "bitmaps/smile_ohno"
#include "bitmaps/smile_stress"
#include "bitmaps/smile_sleep"


//-----------------------------------------------------------------------------
const char **Smiley::XPM_NAMES[Smiley::NbPixmaps] = {
    smile_xpm, smile_stress_xpm, smile_happy_xpm, smile_ohno_xpm,
    smile_sleep_xpm
};

void Smiley::setMood(Mood mood)
{
    QPixmap p(XPM_NAMES[mood]);
    setPixmap(p);
}

//-----------------------------------------------------------------------------
LCDNumber::LCDNumber(QWidget *parent, const char *name)
: QLCDNumber(parent, name)
{
	setFrameStyle( QFrame::Panel | QFrame::Sunken );
	setSegmentStyle(Flat);
    setColor(white);
}

void LCDNumber::setColor(QColor color)
{
	QPalette p = palette();
	p.setColor(QColorGroup::Background, black);
    p.setColor(QColorGroup::Foreground, color);
	setPalette(p);
}

//-----------------------------------------------------------------------------
DigitalClock::DigitalClock(QWidget *parent)
: LCDNumber(parent, "digital_clock")
{}

KExtHighscores::Score DigitalClock::score() const
{
    KExtHighscores::Score score(KExtHighscores::Won);
    score.setData("score", 3600 - (_min*60 + _sec));
    score.setData("nb_actions", _nbActions);
    return score;
}

void DigitalClock::timerEvent(QTimerEvent *)
{
 	if (_stop) return;

    if ( _min==59 && _sec==59 ) return; // waiting an hour do not restart timer
    _sec++;
    if (_sec==60) {
        _min++;
        _sec = 0;
    }
    showTime();

    if ( _first<score() ) setColor(red);
    else if ( _last<score() ) setColor(blue);
    else setColor(white);
}

void DigitalClock::showTime()
{
	char s[6] = "00:00";
	if (_min>=10) s[0] += _min / 10;
	s[1] += _min % 10;
	if (_sec>=10) s[3] += _sec / 10;
	s[4] += _sec % 10;

	display(s);
}

void DigitalClock::reset(const KExtHighscores::Score &first,
                         const KExtHighscores::Score &last)
{
	killTimers();

	_stop = true;
	_sec = 0;
    _min = 0;
    _nbActions = 0;
    _first = first;
    _last = last;
	startTimer(1000); // one second

	setColor(white);
	showTime();
}

//-----------------------------------------------------------------------------
CustomDialog::CustomDialog(Level &level, QWidget *parent)
: KDialogBase(Plain, i18n("Customize your game"), Ok|Cancel, Cancel,
			  parent, 0, true, true), _level(level)
{
	QVBoxLayout *top = new QVBoxLayout(plainPage(), spacingHint());

	// width
	_width = new KIntNumInput(level.width(), plainPage());
	_width->setLabel(i18n("Width"));
	_width->setRange(Level::MIN_CUSTOM_SIZE, Level::MAX_CUSTOM_SIZE);
	connect(_width, SIGNAL(valueChanged(int)), SLOT(widthChanged(int)));
	top->addWidget(_width);

	// height
	_height = new KIntNumInput(level.height(), plainPage());
	_height->setLabel(i18n("Height"));
	_height->setRange(Level::MIN_CUSTOM_SIZE, Level::MAX_CUSTOM_SIZE);
	connect(_height, SIGNAL(valueChanged(int)), SLOT(heightChanged(int)));
	top->addWidget(_height);

	// mines
	_mines = new KIntNumInput(level.nbMines(), plainPage());
	connect(_mines, SIGNAL(valueChanged(int)), SLOT(nbMinesChanged(int)));
	top->addWidget(_mines);

    // combo to choose game type
    QHBoxLayout *hbox = new QHBoxLayout(top);
    QLabel *label = new QLabel(i18n("Game type :"), plainPage());
    hbox->addWidget(label);
    _gameType = new QComboBox(false, plainPage());
    connect(_gameType, SIGNAL(activated(int)), SLOT(typeChosen(int)));
    for (uint i=0; i<=Level::NbLevels; i++)
        _gameType->insertItem(i18n(Level::data((Level::Type)i).i18nLabel));
    hbox->addWidget(_gameType);

    updateNbMines();
}

void CustomDialog::widthChanged(int n)
{
    _level = Level(n, _level.height(), _level.nbMines());
	updateNbMines();
}

void CustomDialog::heightChanged(int n)
{
    _level = Level(_level.width(), n, _level.nbMines());
	updateNbMines();
}

void CustomDialog::nbMinesChanged(int n)
{
    _level = Level(_level.width(), _level.height(), n);
	updateNbMines();
}

void CustomDialog::updateNbMines()
{
	_mines->setRange(1, _level.maxNbMines());
	uint nb = _level.width() * _level.height();
	_mines->setLabel(i18n("Mines (%1%)").arg(100*_level.nbMines()/nb));
    _gameType->setCurrentItem(_level.type());
}

void CustomDialog::typeChosen(int i)
{
    _level = Level((Level::Type)i);
    _width->setValue(_level.width());
    _height->setValue(_level.height());
    _mines->setValue(_level.nbMines());
}

//-----------------------------------------------------------------------------
class SettingsConfigGroup : public KConfigGroupSaver
{
 public:
    SettingsConfigGroup() : KConfigGroupSaver(kapp->config(), "Options") {}
};

const char *OP_UMARK             = "? mark";
const char *OP_KEYBOARD          = "keyboard game";
const char *OP_PAUSE_FOCUS       = "paused if lose focus";
const char *OP_MOUSE_BINDINGS[3] =
    { "mouse left", "mouse mid", "mouse right" };

GameSettingsWidget::GameSettingsWidget()
    : SettingsWidget(i18n("Game"), "misc")
{
    QVBoxLayout *top = new QVBoxLayout(this, KDialog::spacingHint());

    _umark = new QCheckBox(i18n("Enable ? mark"), this);
	top->addWidget(_umark);

	_keyb = new QCheckBox(i18n("Enable keyboard"), this);
	top->addWidget(_keyb);

    _focus = new QCheckBox(i18n("Pause if window lose focus"), this);
	top->addWidget(_focus);

	top->addSpacing( 2*KDialog::spacingHint() );

	QVGroupBox *gb = new QVGroupBox(i18n("Mouse bindings"), this);
	top->addWidget(gb);
	QGrid *grid = new QGrid(2, gb);
	grid->setSpacing(10);
	(void)new QLabel(i18n("Left button"), grid);
	_cb[KMines::Left] = new QComboBox(false, grid);
	(void)new QLabel(i18n("Mid button"), grid);
	_cb[KMines::Mid] = new QComboBox(false, grid);
	(void)new QLabel(i18n("Right button"), grid);
	_cb[KMines::Right] = new QComboBox(false, grid);

	for (uint i=0; i<3; i++) {
		_cb[i]->insertItem(i18n("reveal"), 0);
		_cb[i]->insertItem(i18n("autoreveal"), 1);
		_cb[i]->insertItem(i18n("toggle flag"), 2);
		_cb[i]->insertItem(i18n("toggle ? mark"), 3);
	}
}

bool GameSettingsWidget::readUMark()
{
    SettingsConfigGroup cg;
	return cg.config()->readBoolEntry(OP_UMARK, true);
}

bool GameSettingsWidget::readKeyboard()
{
    SettingsConfigGroup cg;
	return cg.config()->readBoolEntry(OP_KEYBOARD, false);
}

bool GameSettingsWidget::readPauseFocus()
{
    SettingsConfigGroup cg;
	return cg.config()->readBoolEntry(OP_PAUSE_FOCUS, true);
}

KMines::MouseAction GameSettingsWidget::readMouseBinding(MouseButton mb)
{
    SettingsConfigGroup cg;
	MouseAction ma = (MouseAction)cg.config()
                     ->readUnsignedNumEntry(OP_MOUSE_BINDINGS[mb], mb);
	return ma>UMark ? Reveal : ma;
}

void GameSettingsWidget::load()
{
    _umark->setChecked( readUMark() );
    _keyb->setChecked( readKeyboard() );
    _focus->setChecked( readPauseFocus() );
    for (uint i=0; i<3; i++)
        _cb[i]->setCurrentItem( readMouseBinding((MouseButton)i) );
}

void GameSettingsWidget::save()
{
    SettingsConfigGroup cg;
	cg.config()->writeEntry(OP_UMARK, _umark->isChecked());
	cg.config()->writeEntry(OP_KEYBOARD, _keyb->isChecked());
    cg.config()->writeEntry(OP_PAUSE_FOCUS, _focus->isChecked());
	for (uint i=0; i<3; i++)
		cg.config()->writeEntry(OP_MOUSE_BINDINGS[i], _cb[i]->currentItem());
}

void GameSettingsWidget::defaults()
{
    _umark->setChecked(true);
    _keyb->setChecked(false);
    _focus->setChecked(true);
    for (uint i=0; i<3; i++) _cb[i]->setCurrentItem(i);
}

//-----------------------------------------------------------------------------
const char *OP_CASE_SIZE         = "case size";
const uint MIN_CASE_SIZE     = 20;
const uint DEFAULT_CASE_SIZE = MIN_CASE_SIZE;
const uint MAX_CASE_SIZE     = 100;
const char *OP_NUMBER_COLOR    = "color #";
const char *OP_FLAG_COLOR      = "flag color";
const char *OP_EXPLOSION_COLOR = "explosion color";
const char *OP_ERROR_COLOR     = "error color";
#define NCName(i) QString("%1%2").arg(OP_NUMBER_COLOR).arg(i)
const QColor DEFAULT_NUMBER_COLORS[NB_NUMBER_COLORS] =
   { Qt::blue, Qt::darkGreen, Qt::darkYellow, Qt::darkMagenta, Qt::red,
	 Qt::darkRed, Qt::black, Qt::black };
const QColor DEFAULT_FLAG_COLOR      = Qt::red;
const QColor DEFAULT_EXPLOSION_COLOR = Qt::red;
const QColor DEFAULT_ERROR_COLOR     = Qt::red;

AppearanceSettingsWidget::AppearanceSettingsWidget()
    : SettingsWidget(i18n("Appearance"), "appearance")
{
    QVBoxLayout *top = new QVBoxLayout(this, KDialog::spacingHint());

    QHBox *hbox = new QHBox(this);
    hbox->setSpacing(KDialog::spacingHint());
    top->addWidget(hbox);
    (void)new QLabel(i18n("Case size"), hbox);
    _caseSize = new KIntNumInput(hbox);
	_caseSize->setRange(MIN_CASE_SIZE, MAX_CASE_SIZE);

    top->addSpacing( 2*KDialog::spacingHint() );

    QGrid *grid = new QGrid(2, this);
    top->addWidget(grid);

    (void)new QLabel(i18n("Flag color"), grid);
	_flag = new KColorButton(grid);
    _flag->setFixedWidth(100);

	(void)new QLabel(i18n("Explosion color"), grid);
	_explosion = new KColorButton(grid);
    _explosion->setFixedWidth(100);

	(void)new QLabel(i18n("Error color"), grid);
	_error = new KColorButton(grid);
    _error->setFixedWidth(100);

	_numbers.resize(NB_NUMBER_COLORS);
	for (uint i=0; i<NB_NUMBER_COLORS; i++) {
		(void)new QLabel(i==0 ? i18n("One mine color")
						 : i18n("%1 mines color").arg(i+1), grid);
        KColorButton *b = new KColorButton(grid);
        b->setFixedWidth(100);
		_numbers.insert(i, b);
	}
}

uint AppearanceSettingsWidget::readCaseSize()
{
    SettingsConfigGroup cg;
	uint cs
        = cg.config()->readUnsignedNumEntry(OP_CASE_SIZE, DEFAULT_CASE_SIZE);
	cs = QMAX(QMIN(cs, MAX_CASE_SIZE), MIN_CASE_SIZE);
	return cs;
}

void AppearanceSettingsWidget::writeCaseSize(uint size)
{
    SettingsConfigGroup cg;
    cg.config()->writeEntry(OP_CASE_SIZE, size);
}

QColor AppearanceSettingsWidget::readColor(const QString & key,
                                           QColor defaultColor)
{
    SettingsConfigGroup cg;
	return cg.config()->readColorEntry(key, &defaultColor);
}

KMines::CaseProperties AppearanceSettingsWidget::readCaseProperties()
{
	CaseProperties cp;
	cp.size = readCaseSize();
	cp.flagColor = readColor(OP_FLAG_COLOR, DEFAULT_FLAG_COLOR);
	cp.explosionColor
        = readColor(OP_EXPLOSION_COLOR, DEFAULT_EXPLOSION_COLOR);
	cp.errorColor
        = readColor(OP_ERROR_COLOR, DEFAULT_ERROR_COLOR);
	for (uint i=0; i<NB_NUMBER_COLORS; i++)
		cp.numberColors[i]
            = readColor(NCName(i), DEFAULT_NUMBER_COLORS[i]);
	return cp;
}

void AppearanceSettingsWidget::load()
{
    CaseProperties cp = readCaseProperties();
    _caseSize->setValue(cp.size);
    _flag->setColor(cp.flagColor);
    _explosion->setColor(cp.explosionColor);
    _error->setColor(cp.errorColor);
    for (uint i=0; i<NB_NUMBER_COLORS; i++)
        _numbers[i]->setColor(cp.numberColors[i]);
}

void AppearanceSettingsWidget::save()
{
    writeCaseSize( _caseSize->value() );
    SettingsConfigGroup cg;
    cg.config()->writeEntry(OP_FLAG_COLOR, _flag->color());
	cg.config()->writeEntry(OP_EXPLOSION_COLOR, _explosion->color());
	cg.config()->writeEntry(OP_ERROR_COLOR, _error->color());
	for (uint i=0; i<NB_NUMBER_COLORS; i++)
		cg.config()->writeEntry(NCName(i), _numbers[i]->color());
}

void AppearanceSettingsWidget::defaults()
{
    _caseSize->setValue(DEFAULT_CASE_SIZE);
    _flag->setColor(DEFAULT_FLAG_COLOR);
	_explosion->setColor(DEFAULT_EXPLOSION_COLOR);
	_error->setColor(DEFAULT_ERROR_COLOR);
	for (uint i=0; i<NB_NUMBER_COLORS; i++)
		_numbers[i]->setColor(DEFAULT_NUMBER_COLORS[i]);
}

//-----------------------------------------------------------------------------
const char *OP_MENUBAR         = "menubar visible";
const char *OP_LEVEL           = "Level";
const char *OP_CUSTOM_WIDTH    = "custom width";
const char *OP_CUSTOM_HEIGHT   = "custom height";
const char *OP_CUSTOM_MINES    = "custom mines";

ExtSettingsDialog::ExtSettingsDialog(QWidget *parent)
    : SettingsDialog(parent)
{
    addModule(new GameSettingsWidget);
    addModule(new AppearanceSettingsWidget);
    addModule( kHighscores->createSettingsWidget(this) );
}

Level ExtSettingsDialog::readLevel()
{
    SettingsConfigGroup cg;
	Level::Type type
        = (Level::Type)cg.config()->readUnsignedNumEntry(OP_LEVEL, 0);
    if ( type>Level::Custom ) type = Level::Easy;
    if ( type!=Level::Custom ) return Level(type);

    uint width = cg.config()->readUnsignedNumEntry(OP_CUSTOM_WIDTH, 0);
    uint height = cg.config()->readUnsignedNumEntry(OP_CUSTOM_HEIGHT, 0);
    uint nbMines = cg.config()->readUnsignedNumEntry(OP_CUSTOM_MINES, 0);
    return Level(width, height, nbMines);
}

void ExtSettingsDialog::writeLevel(const Level &level)
{
    SettingsConfigGroup cg;
	if ( level.type()==Level::Custom ) {
		cg.config()->writeEntry(OP_CUSTOM_WIDTH, level.width());
		cg.config()->writeEntry(OP_CUSTOM_HEIGHT, level.height());
		cg.config()->writeEntry(OP_CUSTOM_MINES, level.nbMines());
	}
	cg.config()->writeEntry(OP_LEVEL, (uint)level.type());
}

bool ExtSettingsDialog::readMenuVisible()
{
    SettingsConfigGroup cg;
	return cg.config()->readBoolEntry(OP_MENUBAR, true);
}

void ExtSettingsDialog::writeMenuVisible(bool visible)
{
    SettingsConfigGroup cg;
	cg.config()->writeEntry(OP_MENUBAR, visible);
}
