#include <QDir>
#include <QFileDialog>
#include <QFontDialog>
#include <QColorDialog>
#include <QAbstractButton>
#include <QMessageBox>

#include "../Config.h"
#include "ui_configDialog.h"
#include "Settings.h"
#include "ConfigDialog.h"
#include "FullscreenResolutions.h"

static
struct
{
	unsigned short width, height;
	const char *description;
} WindowedModes[] = {
	{ 320, 240, "320 x 240" },
	{ 400, 300, "400 x 300" },
	{ 480, 360, "480 x 360" },
	{ 640, 480, "640 x 480" },
	{ 800, 600, "800 x 600" },
	{ 960, 720, "960 x 720" },
	{ 1024, 768, "1024 x 768" },
	{ 1152, 864, "1152 x 864" },
	{ 1280, 960, "1280 x 960" },
	{ 1280, 1024, "1280 x 1024" },
	{ 1440, 1080, "1440 x 1080" },
	{ 1600, 1024, "1600 x 1024" },
	{ 1600, 1200, "1600 x 1200" },
	{ 640, 480, "custom" }
};
static
const unsigned int numWindowedModes = sizeof(WindowedModes) / sizeof(WindowedModes[0]);

static const unsigned int numFilters = 7U;
static const char * cmbTexFilter_choices[numFilters] = {
	"None",
	"Smooth filtering 1",
	"Smooth filtering 2",
	"Smooth filtering 3",
	"Smooth filtering 4",
	"Sharp filtering 1",
	"Sharp filtering 2"
};

static const unsigned int numEnhancements = 14U;
static const char * cmbTexEnhancement_choices[numEnhancements] = {
	"None",
	"Store",
	"X2",
	"X2SAI",
	"HQ2X",
	"HQ2XS",
	"LQ2X",
	"LQ2XS",
	"HQ4X",
	"2xBRZ",
	"3xBRZ",
	"4xBRZ",
	"5xBRZ",
	"6xBRZ"
};

void ConfigDialog::_init()
{
	// Video settings
	QStringList windowedModesList;
	const int windowedModesCustom = numWindowedModes - 1;
	int windowedModesCurrent = windowedModesCustom;
	for (int i = 0; i < numWindowedModes; ++i) {
		windowedModesList.append(WindowedModes[i].description);
		if (i != windowedModesCustom &&
			WindowedModes[i].width == config.video.windowedWidth &&
			WindowedModes[i].height == config.video.windowedHeight)
			windowedModesCurrent = i;
	}
	ui->windowedResolutionComboBox->insertItems(0, windowedModesList);
	ui->windowedResolutionComboBox->setCurrentIndex(windowedModesCurrent);

	QStringList fullscreenModesList, fullscreenRatesList;
	int fullscreenMode, fullscreenRate;
	fillFullscreenResolutionsList(fullscreenModesList, fullscreenMode, fullscreenRatesList, fullscreenRate);
	ui->fullScreenResolutionComboBox->insertItems(0, fullscreenModesList);
	ui->fullScreenResolutionComboBox->setCurrentIndex(fullscreenMode);
	ui->fullScreenRefreshRateComboBox->insertItems(0, fullscreenRatesList);
	ui->fullScreenRefreshRateComboBox->setCurrentIndex(fullscreenRate);

	ui->aliasingSlider->setValue(config.video.multisampling);
	ui->anisotropicSlider->setValue(config.texture.maxAnisotropy);
	ui->cacheSizeSpinBox->setValue(config.texture.maxBytes / gc_uMegabyte);

	switch (config.texture.bilinearMode) {
	case BILINEAR_3POINT:
		ui->blnr3PointRadioButton->setChecked(true);
		break;
	case BILINEAR_STANDARD:
		ui->blnrStandardRadioButton->setChecked(true);
		break;
	}

	switch (config.texture.screenShotFormat) {
	case 0:
		ui->bmpRadioButton->setChecked(true);
		break;
	case 1:
		ui->jpegRadioButton->setChecked(true);
		break;
	}

	// Emulation settings
	ui->emulateLodCheckBox->setChecked(config.generalEmulation.enableLOD != 0);
	ui->emulateNoiseCheckBox->setChecked(config.generalEmulation.enableNoise != 0);
	ui->enableHWLightingCheckBox->setChecked(config.generalEmulation.enableHWLighting != 0);
	ui->enableShadersStorageCheckBox->setChecked(config.generalEmulation.enableShadersStorage != 0);
	ui->customSettingsCheckBox->setChecked(config.generalEmulation.enableCustomSettings != 0);
	switch (config.generalEmulation.correctTexrectCoords) {
	case Config::tcDisable:
		ui->fixTexrectDisableRadioButton->setChecked(true);
		break;
	case Config::tcSmart:
		ui->fixTexrectSmartRadioButton->setChecked(true);
		break;
	case Config::tcForce:
		ui->fixTexrectForceRadioButton->setChecked(true);
		break;
	}
	ui->nativeRes2D_checkBox->setChecked(config.generalEmulation.enableNativeResTexrects != 0);
	if (ui->nativeRes2D_checkBox->isChecked()) {
		ui->texrectsBlackLinesLabel->setEnabled(false);
		ui->fixTexrectDisableRadioButton->setEnabled(false);
		ui->fixTexrectSmartRadioButton->setEnabled(false);
		ui->fixTexrectForceRadioButton->setEnabled(false);
	}

	ui->bufferSwapComboBox->setCurrentIndex(config.frameBufferEmulation.bufferSwapMode);
	ui->frameBufferGroupBox->setChecked(config.frameBufferEmulation.enable != 0);
	switch (config.frameBufferEmulation.copyToRDRAM) {
	case Config::ctDisable:
		ui->copyBufferDisableRadioButton->setChecked(true);
		break;
	case Config::ctSync:
		ui->copyBufferSyncRadioButton->setChecked(true);
		break;
	case Config::ctAsync:
		ui->copyBufferAsyncRadioButton->setChecked(true);
		break;
	}
	ui->RenderFBCheckBox->setChecked(config.frameBufferEmulation.copyFromRDRAM != 0);
	switch (config.frameBufferEmulation.copyDepthToRDRAM) {
	case Config::cdDisable:
		ui->copyDepthDisableRadioButton->setChecked(true);
		break;
	case Config::cdCopyFromVRam:
		ui->copyDepthVRamRadioButton->setChecked(true);
		break;
	case Config::cdSoftwareRender:
		ui->copyDepthSoftwareRadioButton->setChecked(true);
		break;
	}
	ui->n64DepthCompareCheckBox->setChecked(config.frameBufferEmulation.N64DepthCompare != 0);
	switch (config.frameBufferEmulation.aspect) {
	case Config::aStretch:
		ui->aspectStretchRadioButton->setChecked(true);
		break;
	case Config::a43:
		ui->aspect43RadioButton->setChecked(true);
		break;
	case Config::a169:
		ui->aspect169RadioButton->setChecked(true);
		break;
	case Config::aAdjust:
		ui->aspectAdjustRadioButton->setChecked(true);
		break;
	}
	ui->resolutionFactorSlider->setValue(config.frameBufferEmulation.nativeResFactor);
	ui->copyAuxBuffersCheckBox->setChecked(config.frameBufferEmulation.copyAuxToRDRAM != 0);
	ui->fbInfoDisableCheckBox->setChecked(config.frameBufferEmulation.fbInfoDisabled != 0);
	ui->readColorChunkCheckBox->setChecked(config.frameBufferEmulation.fbInfoReadColorChunk != 0);
	ui->readColorChunkCheckBox->setEnabled(config.frameBufferEmulation.fbInfoDisabled == 0);
	ui->readDepthChunkCheckBox->setChecked(config.frameBufferEmulation.fbInfoReadDepthChunk != 0);
	ui->readDepthChunkCheckBox->setEnabled(config.frameBufferEmulation.fbInfoDisabled == 0);

	// Texture filter settings
	QStringList textureFiltersList;
	for (unsigned int i = 0; i < numFilters; ++i)
		textureFiltersList.append(cmbTexFilter_choices[i]);
	ui->filterComboBox->insertItems(0, textureFiltersList);
	ui->filterComboBox->setCurrentIndex(config.textureFilter.txFilterMode);

	QStringList textureEnhancementList;
	for (unsigned int i = 0; i < numEnhancements; ++i)
		textureEnhancementList.append(cmbTexEnhancement_choices[i]);
	ui->enhancementComboBox->insertItems(0, textureEnhancementList);
	ui->enhancementComboBox->setCurrentIndex(config.textureFilter.txEnhancementMode);

	ui->textureFilterCacheSpinBox->setValue(config.textureFilter.txCacheSize / gc_uMegabyte);
	ui->deposterizeCheckBox->setChecked(config.textureFilter.txDeposterize != 0);
	ui->ignoreBackgroundsCheckBox->setChecked(config.textureFilter.txFilterIgnoreBG != 0);

	ui->texturePackGroupBox->setChecked(config.textureFilter.txHiresEnable != 0);
	ui->alphaChannelCheckBox->setChecked(config.textureFilter.txHiresFullAlphaChannel != 0);
	ui->alternativeCRCCheckBox->setChecked(config.textureFilter.txHresAltCRC != 0);
	ui->textureDumpCheckBox->setChecked(config.textureFilter.txDump != 0);
	ui->force16bppCheckBox->setChecked(config.textureFilter.txForce16bpp != 0);
	ui->compressCacheCheckBox->setChecked(config.textureFilter.txCacheCompression != 0);
	ui->saveTextureCacheCheckBox->setChecked(config.textureFilter.txSaveCache != 0);

	ui->txPathLabel->setText(QString::fromWCharArray(config.textureFilter.txPath));

	QString fontName(config.font.name.c_str());
	m_font = QFont(fontName.left(fontName.indexOf(".ttf")), config.font.size);
	QString strSize;
	strSize.setNum(m_font.pointSize());
	ui->fontNameLabel->setText(m_font.family() + " - " + strSize);

	m_color = QColor(config.font.color[0], config.font.color[1], config.font.color[2]);
	ui->fontColorLabel->setFont(m_font);
	ui->fontColorLabel->setText(m_color.name());
	QPalette palette;
	palette.setColor(QPalette::Window, Qt::black);
	palette.setColor(QPalette::WindowText, m_color);
	ui->fontColorLabel->setAutoFillBackground(true);
	ui->fontColorLabel->setPalette(palette);

	// Post filter settings
	ui->bloomGroupBox->setChecked(config.bloomFilter.enable != 0);
	switch (config.bloomFilter.blendMode) {
	case 0:
		ui->bloomStrongRadioButton->setChecked(true);
		break;
	case 1:
		ui->bloomMildRadioButton->setChecked(true);
		break;
	case 2:
		ui->bloomLightRadioButton->setChecked(true);
		break;
	}
	ui->bloomThresholdSlider->setValue(config.bloomFilter.thresholdLevel);
	ui->blurAmountSlider->setValue(config.bloomFilter.blurAmount);
	ui->blurStrengthSlider->setValue(config.bloomFilter.blurStrength);

	ui->forceGammaCorrectionCheckBox->setChecked(config.gammaCorrection.force  != 0);
	ui->gammaLevelSpinBox->setValue(config.gammaCorrection.level);
	ui->gammaLevelSpinBox->setEnabled(ui->forceGammaCorrectionCheckBox->isChecked());
}

void ConfigDialog::_getTranslations(QStringList & _translationFiles) const
{
	QDir pluginFolder(m_strIniPath);
	QStringList nameFilters("gliden64_*.qm");
	_translationFiles = pluginFolder.entryList(nameFilters, QDir::Files, QDir::Name);
}


void ConfigDialog::setIniPath(const QString & _strIniPath)
{
	m_strIniPath = _strIniPath;

	QStringList translationFiles;
	_getTranslations(translationFiles);

	const QString currentTranslation = getTranslationFile();
	int listIndex = 0;
	QStringList translationLanguages("English");
	for (int i = 0; i < translationFiles.size(); ++i) {
		// get locale extracted by filename
		QString locale = translationFiles[i]; // "TranslationExample_de.qm"
		const bool bCurrent = locale == currentTranslation;
		locale.truncate(locale.lastIndexOf('.')); // "TranslationExample_de"
		locale.remove(0, locale.indexOf('_') + 1); // "de"
		QString language = QLocale::languageToString(QLocale(locale).language());
		if (bCurrent) {
			listIndex = i + 1;
		}
		translationLanguages << language;
	}

	ui->translationsComboBox->insertItems(0, translationLanguages);
	ui->translationsComboBox->setCurrentIndex(listIndex);
}

ConfigDialog::ConfigDialog(QWidget *parent) :
QDialog(parent),
ui(new Ui::ConfigDialog),
m_accepted(false)
{
	ui->setupUi(this);
	_init();
}

ConfigDialog::~ConfigDialog()
{
	delete ui;
}

void ConfigDialog::accept()
{
	m_accepted = true;

	config.video.windowedWidth = ui->windowWidthSpinBox->value();
	config.video.windowedHeight = ui->windowHeightSpinBox->value();

	getFullscreenResolutions(ui->fullScreenResolutionComboBox->currentIndex(), config.video.fullscreenWidth, config.video.fullscreenHeight);
	getFullscreenRefreshRate(ui->fullScreenRefreshRateComboBox->currentIndex(), config.video.fullscreenRefresh);

	config.video.multisampling = ui->aliasingSlider->value();
	config.texture.maxAnisotropy = ui->anisotropicSlider->value();
	config.texture.maxBytes = ui->cacheSizeSpinBox->value() * gc_uMegabyte;

	if (ui->blnrStandardRadioButton->isChecked())
		config.texture.bilinearMode = BILINEAR_STANDARD;
	else if (ui->blnr3PointRadioButton->isChecked())
		config.texture.bilinearMode = BILINEAR_3POINT;

	if (ui->bmpRadioButton->isChecked())
		config.texture.screenShotFormat = 0;
	else if (ui->jpegRadioButton->isChecked())
		config.texture.screenShotFormat = 1;

	const int lanuageIndex = ui->translationsComboBox->currentIndex();
	if (lanuageIndex == 0) // English
		config.translationFile.clear();
	else {
		QStringList translationFiles;
		_getTranslations(translationFiles);
		config.translationFile = translationFiles[lanuageIndex-1].toLocal8Bit().constData();
	}

	// Emulation settings
	config.generalEmulation.enableLOD = ui->emulateLodCheckBox->isChecked() ? 1 : 0;
	config.generalEmulation.enableNoise = ui->emulateNoiseCheckBox->isChecked() ? 1 : 0;
	config.generalEmulation.enableHWLighting = ui->enableHWLightingCheckBox->isChecked() ? 1 : 0;
	config.generalEmulation.enableShadersStorage = ui->enableShadersStorageCheckBox->isChecked() ? 1 : 0;
	config.generalEmulation.enableCustomSettings = ui->customSettingsCheckBox->isChecked() ? 1 : 0;
	if (ui->fixTexrectDisableRadioButton->isChecked())
		config.generalEmulation.correctTexrectCoords = Config::tcDisable;
	else if (ui->fixTexrectSmartRadioButton->isChecked())
		config.generalEmulation.correctTexrectCoords = Config::tcSmart;
	else if (ui->fixTexrectForceRadioButton->isChecked())
		config.generalEmulation.correctTexrectCoords = Config::tcForce;
	config.generalEmulation.enableNativeResTexrects = ui->nativeRes2D_checkBox->isChecked() ? 1 : 0;

	config.frameBufferEmulation.bufferSwapMode = ui->bufferSwapComboBox->currentIndex();
	config.frameBufferEmulation.enable = ui->frameBufferGroupBox->isChecked() ? 1 : 0;
	if (ui->copyBufferDisableRadioButton->isChecked())
		config.frameBufferEmulation.copyToRDRAM = Config::ctDisable;
	else if (ui->copyBufferSyncRadioButton->isChecked())
		config.frameBufferEmulation.copyToRDRAM = Config::ctSync;
	else if (ui->copyBufferAsyncRadioButton->isChecked())
		config.frameBufferEmulation.copyToRDRAM = Config::ctAsync;
	config.frameBufferEmulation.copyFromRDRAM = ui->RenderFBCheckBox->isChecked() ? 1 : 0;
	if (ui->copyDepthDisableRadioButton->isChecked())
		config.frameBufferEmulation.copyDepthToRDRAM = Config::cdDisable;
	else if (ui->copyDepthVRamRadioButton->isChecked())
		config.frameBufferEmulation.copyDepthToRDRAM = Config::cdCopyFromVRam;
	else if (ui->copyDepthSoftwareRadioButton->isChecked())
		config.frameBufferEmulation.copyDepthToRDRAM = Config::cdSoftwareRender;
	config.frameBufferEmulation.N64DepthCompare = ui->n64DepthCompareCheckBox->isChecked() ? 1 : 0;
	if (ui->aspectStretchRadioButton->isChecked())
		config.frameBufferEmulation.aspect = Config::aStretch;
	else if (ui->aspect43RadioButton->isChecked())
		config.frameBufferEmulation.aspect = Config::a43;
	else if (ui->aspect169RadioButton->isChecked())
		config.frameBufferEmulation.aspect = Config::a169;
	else if (ui->aspectAdjustRadioButton->isChecked())
		config.frameBufferEmulation.aspect = Config::aAdjust;
	config.frameBufferEmulation.nativeResFactor = ui->resolutionFactorSlider->value();
	config.frameBufferEmulation.copyAuxToRDRAM = ui->copyAuxBuffersCheckBox->isChecked() ? 1 : 0;
	config.frameBufferEmulation.fbInfoDisabled = ui->fbInfoDisableCheckBox->isChecked() ? 1: 0;
	config.frameBufferEmulation.fbInfoReadColorChunk = ui->readColorChunkCheckBox->isChecked() ? 1 : 0;
	config.frameBufferEmulation.fbInfoReadDepthChunk = ui->readDepthChunkCheckBox->isChecked() ? 1 : 0;

	// Texture filter settings
	config.textureFilter.txFilterMode = ui->filterComboBox->currentIndex();
	config.textureFilter.txEnhancementMode = ui->enhancementComboBox->currentIndex();

	config.textureFilter.txCacheSize = ui->textureFilterCacheSpinBox->value() * gc_uMegabyte;
	config.textureFilter.txDeposterize = ui->deposterizeCheckBox->isChecked() ? 1 : 0;
	config.textureFilter.txFilterIgnoreBG = ui->ignoreBackgroundsCheckBox->isChecked() ? 1 : 0;

	config.textureFilter.txHiresEnable = ui->texturePackGroupBox->isChecked() ? 1 : 0;
	config.textureFilter.txHiresFullAlphaChannel = ui->alphaChannelCheckBox->isChecked() ? 1 : 0;
	config.textureFilter.txHresAltCRC = ui->alternativeCRCCheckBox->isChecked() ? 1 : 0;
	config.textureFilter.txDump = ui->textureDumpCheckBox->isChecked() ? 1 : 0;

	config.textureFilter.txCacheCompression = ui->compressCacheCheckBox->isChecked() ? 1 : 0;
	config.textureFilter.txForce16bpp = ui->force16bppCheckBox->isChecked() ? 1 : 0;
	config.textureFilter.txSaveCache = ui->saveTextureCacheCheckBox->isChecked() ? 1 : 0;

	QString txPath = ui->txPathLabel->text();
	if (!txPath.isEmpty())
		config.textureFilter.txPath[txPath.toWCharArray(config.textureFilter.txPath)] = L'\0';

	config.font.size = m_font.pointSize();
	QString fontName = m_font.family() + ".ttf";
#ifdef OS_WINDOWS
	config.font.name = fontName.toLocal8Bit().constData();
#else
	config.font.name = fontName.toStdString();
#endif
	config.font.color[0] = m_color.red();
	config.font.color[1] = m_color.green();
	config.font.color[2] = m_color.blue();
	config.font.color[3] = m_color.alpha();
	config.font.colorf[0] = m_color.redF();
	config.font.colorf[1] = m_color.greenF();
	config.font.colorf[2] = m_color.blueF();
	config.font.colorf[3] = m_color.alphaF();

	// Post filter settings
	config.bloomFilter.enable = ui->bloomGroupBox->isChecked() ? 1 : 0;
	if (ui->bloomStrongRadioButton->isChecked())
		config.bloomFilter.blendMode = 0;
	else if (ui->bloomMildRadioButton->isChecked())
		config.bloomFilter.blendMode = 1;
	else if (ui->bloomLightRadioButton->isChecked())
		config.bloomFilter.blendMode = 2;
	config.bloomFilter.thresholdLevel = ui->bloomThresholdSlider->value();
	config.bloomFilter.blurAmount = ui->blurAmountSlider->value();
	config.bloomFilter.blurStrength = ui->blurStrengthSlider->value();

	config.gammaCorrection.force = ui->forceGammaCorrectionCheckBox->isChecked() ? 1 : 0;
	config.gammaCorrection.level = ui->gammaLevelSpinBox->value();

	writeSettings(m_strIniPath);

	QDialog::accept();
}

void ConfigDialog::on_selectFontButton_clicked()
{
	bool ok;
	m_font = QFontDialog::getFont(
		&ok, m_font, this);
	if (!ok)
		return;

	// the user clicked OK and font is set to the font the user selected
	QString strSize;
	strSize.setNum(m_font.pointSize());
	ui->fontNameLabel->setText(m_font.family() + " - " + strSize);
	ui->fontColorLabel->setFont(m_font);
}

void ConfigDialog::on_PickFontColorButton_clicked()
{
	const QColor color = QColorDialog::getColor(m_color, this);

	if (!color.isValid())
		return;

	m_color = color;
	QPalette palette;
	palette.setColor(QPalette::Window, Qt::black);
	palette.setColor(QPalette::WindowText, m_color);
	ui->fontColorLabel->setText(m_color.name());
	ui->fontColorLabel->setPalette(palette);
}

void ConfigDialog::on_buttonBox_clicked(QAbstractButton *button)
{
	if ((QPushButton *)button == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)) {
		QMessageBox msgBox(QMessageBox::Question, "GLideN64",
			"Do you really want to reset all settings to defaults?",
			QMessageBox::RestoreDefaults | QMessageBox::Cancel, this
			);
		msgBox.setDefaultButton(QMessageBox::Cancel);
		if (msgBox.exec() == QMessageBox::RestoreDefaults) {
			config.resetToDefaults();
			_init();
		}
	}
}

void ConfigDialog::on_fullScreenResolutionComboBox_currentIndexChanged(int index)
{
	QStringList fullscreenRatesList;
	int fullscreenRate;
	fillFullscreenRefreshRateList(index, fullscreenRatesList, fullscreenRate);
	ui->fullScreenRefreshRateComboBox->clear();
	ui->fullScreenRefreshRateComboBox->insertItems(0, fullscreenRatesList);
	ui->fullScreenRefreshRateComboBox->setCurrentIndex(fullscreenRate);
}

void ConfigDialog::on_texPackPathButton_clicked()
{
	QFileDialog::Options options = QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly | QFileDialog::ReadOnly | QFileDialog::DontUseSheet | QFileDialog::ReadOnly | QFileDialog::HideNameFilterDetails;
	QString directory = QFileDialog::getExistingDirectory(this,
		"",
		ui->txPathLabel->text(),
		options);
	if (!directory.isEmpty())
		ui->txPathLabel->setText(directory);
}

void ConfigDialog::on_fbInfoDisableCheckBox_toggled(bool checked)
{
	ui->readColorChunkCheckBox->setEnabled(!checked);
	ui->readDepthChunkCheckBox->setEnabled(!checked);
}

void ConfigDialog::on_windowedResolutionComboBox_currentIndexChanged(int index)
{
	const bool bCustom = index == numWindowedModes - 1;
	ui->windowWidthSpinBox->setValue(bCustom ? config.video.windowedWidth : WindowedModes[index].width);
	ui->windowWidthSpinBox->setEnabled(bCustom);
	ui->windowHeightSpinBox->setValue(bCustom ? config.video.windowedHeight : WindowedModes[index].height);
	ui->windowHeightSpinBox->setEnabled(bCustom);
}

void ConfigDialog::on_nativeRes2D_checkBox_toggled(bool checked)
{
	ui->texrectsBlackLinesLabel->setEnabled(!checked);
	ui->fixTexrectDisableRadioButton->setEnabled(!checked);
	ui->fixTexrectSmartRadioButton->setEnabled(!checked);
	ui->fixTexrectForceRadioButton->setEnabled(!checked);
}
