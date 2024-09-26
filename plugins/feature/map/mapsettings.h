///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2015-2020, 2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>    //
// Copyright (C) 2020-2024 Jon Beniston, M7RCE <jon@beniston.com>                //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
// (at your option) any later version.                                           //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDE_FEATURE_MAPSETTINGS_H_
#define INCLUDE_FEATURE_MAPSETTINGS_H_

#include <QByteArray>
#include <QString>
#include <QHash>
#include <QRegularExpression>

#define MAX_FILTER_DISTANCE_KM 10000

class Serializable;

struct MapSettings
{
    struct MapItemSettings {
        QString m_group;          // Name of the group the settings apply to
        bool m_enabled;           // Whether enabled at all on 2D or 3D map
        bool m_display2DIcon;     // Display image 2D map
        bool m_display2DLabel;    // Display label on 2D map
        bool m_display2DTrack;    // Display tracks (or polygon) on 2D map
        quint32 m_2DTrackColor;
        int m_2DMinZoom;
        bool m_display3DModel;    // Draw 3D model for item
        bool m_display3DLabel;    // Display a label next to this item on the 3D map
        bool m_display3DPoint;    // Draw a point for this item on the 3D map
        quint32 m_3DPointColor;
        bool m_display3DTrack;    // Display a ground track for this item on the 3D map
        quint32 m_3DTrackColor;
        int m_3DModelMinPixelSize;
        float m_3DLabelScale;
        QString m_filterName;
        QRegularExpression m_filterNameRE;
        int m_filterDistance;     // Filter items > this distance in metres away from My Position. <= 0 don't filter
        int m_extrapolate;        // Extrapolate duration in seconds on 3D map

        MapItemSettings(const QString& group, bool enabled, const QColor color, bool display2DTrack=true, bool display3DPoint=true, int minZoom=11, int modelMinPixelSize=0);
        MapItemSettings(const QByteArray& data);
        void resetToDefaults();
        QByteArray serialize() const;
        bool deserialize(const QByteArray& data);
    };

    struct MapPolygonSettings {
        QString m_group;
        bool m_enabled;
        quint32 m_2DColor;
        int m_2DMinZoom;
        quint32 m_3DColor;
    };

    bool m_displayNames;
    QString m_mapProvider;
    QString m_thunderforestAPIKey;
    QString m_maptilerAPIKey;
    QString m_mapBoxAPIKey;
    QString m_osmURL;
    QString m_mapBoxStyles;
    bool m_displayAllGroundTracks;
    bool m_displaySelectedGroundTracks;
    QString m_title;
    quint32 m_rgbColor;
    bool m_useReverseAPI;
    QString m_reverseAPIAddress;
    uint16_t m_reverseAPIPort;
    uint16_t m_reverseAPIFeatureSetIndex;
    uint16_t m_reverseAPIFeatureIndex;
    Serializable *m_rollupState;
    bool m_map2DEnabled;
    QString m_mapType;          // "Street Map", "Satellite Map", etc.. as selected in combobox
    int m_workspaceIndex;
    QByteArray m_geometryBytes;

    // 3D Map settings
    bool m_map3DEnabled;
    QString m_terrain;          // "Ellipsoid" or "Cesium World Terrain"
    QString m_buildings;        // "None" or "Cesium OSM Buildings"
    QString m_modelURL;         // Base URL for 3D models (Not user settable, as depends on web server port)
    QString m_modelDir;         // Directory to store 3D models (not customizable for now, as ADS-B plugin needs to know)
    bool m_sunLightEnabled;     // Light globe from direction of Sun
    bool m_eciCamera;           // Use ECI instead of ECEF for camera
    QString m_cesiumIonAPIKey;
    QString m_antiAliasing;

    bool m_displayMUF;          // Plot MUF contours
    bool m_displayfoF2;         // Plot foF2 contours
    bool m_displayRain;
    bool m_displayClouds;
    bool m_displaySeaMarks;
    bool m_displayRailways;
    bool m_displayNASAGlobalImagery;
    QString m_nasaGlobalImageryIdentifier;
    int m_nasaGlobalImageryOpacity;

    QString m_checkWXAPIKey;    //!< checkwxapi.com API key

    // Per source settings
    QHash<QString, MapItemSettings *> m_itemSettings;

    MapSettings();
    ~MapSettings();
    void resetToDefaults();
    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
    void setRollupState(Serializable *rollupState) { m_rollupState = rollupState; }
    QByteArray serializeItemSettings(QHash<QString, MapItemSettings *> itemSettings) const;
    void deserializeItemSettings(const QByteArray& data, QHash<QString, MapItemSettings *>& itemSettings);
    void applySettings(const QStringList& settingsKeys, const MapSettings& settings);
    QString getDebugString(const QStringList& settingsKeys, bool force=false) const;

    static const QStringList m_pipeTypes;
    static const QStringList m_pipeURIs;

    static const QStringList m_mapProviders;
    static const QStringList m_mapProviderNames;
};

Q_DECLARE_METATYPE(MapSettings::MapItemSettings *);

#endif // INCLUDE_FEATURE_MAPSETTINGS_H_

