///////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2015-2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>              //
// Copyright (C) 2020, 2022-2023 Jon Beniston, M7RCE <jon@beniston.com>              //
//                                                                                   //
// This program is free software; you can redistribute it and/or modify              //
// it under the terms of the GNU General Public License as published by              //
// the Free Software Foundation as version 3 of the License, or                      //
// (at your option) any later version.                                               //
//                                                                                   //
// This program is distributed in the hope that it will be useful,                   //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                    //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                      //
// GNU General Public License V3 for more details.                                   //
//                                                                                   //
// You should have received a copy of the GNU General Public License                 //
// along with this program. If not, see <http://www.gnu.org/licenses/>.              //
///////////////////////////////////////////////////////////////////////////////////////
#ifndef INCLUDE_SETTINGS_H
#define INCLUDE_SETTINGS_H

#include <QObject>
#include <QString>
#include "device/deviceuserargs.h"
#include "preferences.h"
#include "preset.h"
#include "featuresetpreset.h"
#include "pluginpreset.h"
#include "configuration.h"
#include "export.h"
#include "plugin/pluginmanager.h"

class Command;
class AudioDeviceManager;


class SDRBASE_API MainSettings : public QObject {
	Q_OBJECT
public:
	MainSettings();
	~MainSettings();

	void load();
	void save() const;

	void resetToDefaults();
    void initialize();
	QString getFileLocation() const;
	int getFileFormat() const; //!< see QSettings::Format for the values

    const Preferences& getPreferences() const { return m_preferences; }
    void setPreferences(const Preferences& preferences) { m_preferences = preferences; }

	Preset* newPreset(const QString& group, const QString& description);
    void addPreset(Preset *preset);
	void deletePreset(const Preset* preset);
    QByteArray serializePreset(const Preset* preset) const;
    bool deserializePreset(const QByteArray& blob, Preset* preset);
	int getPresetCount() const { return m_presets.count(); }
	const Preset* getPreset(int index) const { return m_presets[index]; }
	const Preset* getPreset(const QString& groupName, quint64 centerFrequency, const QString& description, const QString& type) const;
	void sortPresets();
	void renamePresetGroup(const QString& oldGroupName, const QString& newGroupName);
	void deletePresetGroup(const QString& groupName);
    void clearPresets();
    const Preset& getWorkingPresetConst() const { return m_workingPreset; }
	Preset* getWorkingPreset() { return &m_workingPreset; }
    QList<Preset*> *getPresets() { return &m_presets; }

    void addCommand(Command *command);
    void deleteCommand(const Command* command);
    int getCommandCount() const { return m_commands.count(); }
    const Command* getCommand(int index) const { return m_commands[index]; }
    const Command* getCommand(const QString& groupName, const QString& description) const;
    void sortCommands();
    void renameCommandGroup(const QString& oldGroupName, const QString& newGroupName);
    void deleteCommandGroup(const QString& groupName);
    void clearCommands();

	FeatureSetPreset* newFeatureSetPreset(const QString& group, const QString& description);
    void addFeatureSetPreset(FeatureSetPreset *preset);
	void deleteFeatureSetPreset(const FeatureSetPreset* preset);
	int getFeatureSetPresetCount() const { return m_featureSetPresets.count(); }
	const FeatureSetPreset* getFeatureSetPreset(int index) const { return m_featureSetPresets[index]; }
	const FeatureSetPreset* getFeatureSetPreset(const QString& groupName, const QString& description) const;
	void sortFeatureSetPresets();
	void renameFeatureSetPresetGroup(const QString& oldGroupName, const QString& newGroupName);
	void deleteFeatureSetPresetGroup(const QString& groupName);
    void clearFeatureSetPresets();
    const FeatureSetPreset& getWorkingFeatureSetPresetConst() const { return m_workingFeatureSetPreset; }
	FeatureSetPreset* getWorkingFeatureSetPreset() { return &m_workingFeatureSetPreset; }
	QList<FeatureSetPreset*> *getFeatureSetPresets() { return &m_featureSetPresets; }

    PluginPreset* newPluginPreset(const QString& group, const QString& description);
    void addPluginPreset(PluginPreset *preset);
    void deletePluginPreset(const PluginPreset* preset);
    int getPluginPresetCount() const { return m_pluginPresets.count(); }
    const PluginPreset* getPluginPreset(int index) const { return m_pluginPresets[index]; }
    const PluginPreset* getPluginPreset(const QString& groupName, const QString& description) const;
    void sortPluginPresets();
    void renamePluginPresetGroup(const QString& oldGroupName, const QString& newGroupName);
    void deletePluginPresetGroup(const QString& groupName);
    void clearPluginPresets();
    const PluginPreset& getWorkingPluginPresetConst() const { return m_workingPluginPreset; }
    PluginPreset* getWorkingPluginPreset() { return &m_workingPluginPreset; }
    QList<PluginPreset*> *getPluginPresets() { return &m_pluginPresets; }

    Configuration* newConfiguration(const QString& group, const QString& description);
    void addConfiguration(Configuration *configuration);
    void deleteConfiguration(const Configuration *configuration);
    QByteArray serializeConfiguration(const Configuration *configuration) const;
    bool deserializeConfiguration(const QByteArray& blob, Configuration *configuration);
    int getConfigurationCount() const { return m_configurations.size(); }
	const Configuration* getConfiguration(int index) const { return m_configurations[index]; }
	const Configuration* getConfiguration(const QString& groupName, const QString& description) const;
    void sortConfigurations();
	void renameConfigurationGroup(const QString& oldGroupName, const QString& newGroupName);
	void deleteConfigurationGroup(const QString& groupName);
    void clearConfigurations();
    const Configuration& getWorkingConfigurationConst() const { return m_workingConfiguration; }
	Configuration* getWorkingConfiguration() { return &m_workingConfiguration; }
	QList<Configuration*> *getConfigurations() { return &m_configurations; }

	const QString& getSourceDevice() const { return m_preferences.getSourceDevice(); }
	void setSourceDevice(const QString& value)
	{
		m_preferences.setSourceDevice(value);
		emit preferenceChanged(Preferences::SourceDevice);
	}

	int getSourceIndex() const { return m_preferences.getSourceIndex(); }
	void setSourceIndex(int value)
	{
		m_preferences.setSourceIndex(value);
		emit preferenceChanged(Preferences::SourceIndex);
	}

	int getSourceItemIndex() const { return m_preferences.getSourceItemIndex(); }
	void setSourceItemIndex(int value)
	{
		m_preferences.setSourceItemIndex(value);
		emit preferenceChanged(Preferences::SourceItemIndex);
	}

	const QString& getAudioType() const { return m_preferences.getAudioType(); }
	void setAudioType(const QString& value)
	{
		m_preferences.setAudioType(value);
		emit preferenceChanged(Preferences::AudioType);
	}

	const QString& getAudioDevice() const { return m_preferences.getAudioDevice(); }
	void setAudioDevice(const QString& value)
	{
		m_preferences.setAudioDevice(value);
		emit preferenceChanged(Preferences::AudioDevice);
	}

    QString getStationName() const { return m_preferences.getStationName(); }
	void setStationName(const QString& name)
	{
		m_preferences.setStationName(name);
		emit preferenceChanged(Preferences::StationName);
	}

	float getLatitude() const { return m_preferences.getLatitude(); }
	void setLatitude(float latitude)
	{
		m_preferences.setLatitude(latitude);
		emit preferenceChanged(Preferences::Latitude);
	}

	float getLongitude() const { return m_preferences.getLongitude(); }
	void setLongitude(float longitude)
	{
		m_preferences.setLongitude(longitude);
		emit preferenceChanged(Preferences::Longitude);
	}

	float getAltitude() const { return m_preferences.getAltitude(); }
	void setAltitude(float altitude)
	{
		m_preferences.setAltitude(altitude);
		emit preferenceChanged(Preferences::Altitude);
	}

    bool getAutoUpdatePosition() const { return m_preferences.getAutoUpdatePosition(); }
    void setAutoUpdatePosition(bool autoUpdatePosition)
    {
        m_preferences.setAutoUpdatePosition(autoUpdatePosition);
    }

    QtMsgType getConsoleMinLogLevel() const { return m_preferences.getConsoleMinLogLevel(); }
	void setConsoleMinLogLevel(const QtMsgType& minLogLevel)
	{
		m_preferences.setConsoleMinLogLevel(minLogLevel);
		emit preferenceChanged(Preferences::ConsoleMinLogLevel);
	}

    QtMsgType getFileMinLogLevel() const { return m_preferences.getFileMinLogLevel(); }
    void setFileMinLogLevel(const QtMsgType& minLogLevel)
	{
		m_preferences.setFileMinLogLevel(minLogLevel);
		emit preferenceChanged(Preferences::FileMinLogLevel);
	}

    bool getUseLogFile() const { return m_preferences.getUseLogFile(); }
	void setUseLogFile(bool useLogFile)
	{
		m_preferences.setUseLogFile(useLogFile);
		emit preferenceChanged(Preferences::UseLogFile);
	}

    const QString& getLogFileName() const { return m_preferences.getLogFileName(); }
	void setLogFileName(const QString& value)
	{
		m_preferences.setLogFileName(value);
		emit preferenceChanged(Preferences::LogFileName);
	}

	DeviceUserArgs& getDeviceUserArgs() { return m_hardwareDeviceUserArgs; }
	const AudioDeviceManager *getAudioDeviceManager() const { return m_audioDeviceManager; }
	void setAudioDeviceManager(AudioDeviceManager *audioDeviceManager) { m_audioDeviceManager = audioDeviceManager; }

    int getMultisampling() const { return m_preferences.getMultisampling(); }
    void setMultisampling(int samples)
    {
        m_preferences.setMultisampling(samples);
        emit preferenceChanged(Preferences::Multisampling);
    }

    int getMapMultisampling() const { return m_preferences.getMapMultisampling(); }
    void setMapMultisampling(int samples)
    {
        m_preferences.setMapMultisampling(samples);
        emit preferenceChanged(Preferences::MapMultisampling);
    }

    bool getMapSmoothing() const { return m_preferences.getMapSmoothing(); }
    void setMapSmoothing(bool smoothing)
    {
        m_preferences.setMapSmoothing(smoothing);
        emit preferenceChanged(Preferences::MapSmoothing);
    }

    const QString& getFFTEngine() const { return m_preferences.getFFTEngine(); }
    void setFFTEngine(const QString& fftEngine)
    {
        m_preferences.setFFTEngine(fftEngine);
        emit preferenceChanged(Preferences::FFTEngine);
    }

signals:
	void preferenceChanged(int);

protected:
	Preferences m_preferences;
	AudioDeviceManager *m_audioDeviceManager;
	Preset m_workingPreset;
	typedef QList<Preset*> Presets;
	Presets m_presets;
    typedef QList<Command*> Commands;
    Commands m_commands;
	FeatureSetPreset m_workingFeatureSetPreset;
    typedef QList<FeatureSetPreset*> FeatureSetPresets;
    FeatureSetPresets m_featureSetPresets;
    PluginPreset m_workingPluginPreset;
    typedef QList<PluginPreset*> PluginPresets;
    PluginPresets m_pluginPresets;
    Configuration m_workingConfiguration;
    typedef QList<Configuration*> Configurations;
    Configurations m_configurations;
	DeviceUserArgs m_hardwareDeviceUserArgs;
};

#endif // INCLUDE_SETTINGS_H
