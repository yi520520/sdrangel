///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023 Edouard Griffiths, F4EXB <f4exb06@gmail.com>               //
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
#include "ft8demodsettings.h"
#include "ft8demodfilterproxy.h"

FT8DemodFilterProxy::FT8DemodFilterProxy(QObject *parent) :
    QSortFilterProxyModel(parent),
    m_filterActive(FILTER_NONE)
{
}

void FT8DemodFilterProxy::resetFilter()
{
    m_filterActive = FILTER_NONE;
    invalidateFilter();
}

void FT8DemodFilterProxy::setFilterUTC(const QString& utcString)
{
    m_filterActive = FILTER_UTC;
    m_utc = utcString;
    invalidateFilter();
}

void FT8DemodFilterProxy::setFilterDf(int df)
{
    m_filterActive = FILTER_DF;
    m_df = df;
    invalidateFilter();
}

void FT8DemodFilterProxy::setFilterCall(const QString& callString)
{
    m_filterActive = FILTER_CALL;
    m_call = callString;
    invalidateFilter();
}

void FT8DemodFilterProxy::setFilterLoc(const QString& locString)
{
    m_filterActive = FILTER_LOC;
    m_loc = locString;
    invalidateFilter();
}

void FT8DemodFilterProxy::setFilterCountry(const QString& countryString)
{
    m_filterActive = FILTER_COUNTRY;
    m_country = countryString;
    invalidateFilter();
}

void FT8DemodFilterProxy::setFilterInfo(const QString& infoString)
{
    m_filterActive = FILTER_INFO;
    m_info= infoString;
    invalidateFilter();
}

bool FT8DemodFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (m_filterActive == FILTER_NONE) {
        return true;
    }

    if (m_filterActive == FILTER_UTC)
    {
        QModelIndex index = sourceModel()->index(sourceRow, FT8DemodSettings::MESSAGE_COL_UTC, sourceParent);
        return sourceModel()->data(index).toString() == m_utc;
    }

    if (m_filterActive == FILTER_DF)
    {
        QModelIndex index = sourceModel()->index(sourceRow, FT8DemodSettings::MESSAGE_COL_DF, sourceParent);
        int df = sourceModel()->data(index).toInt();
        return (df >= m_df - 4) && (df <= m_df + 4); // +/- 4 Hz tolerance which is about one symbol width
    }

    if (m_filterActive == FILTER_CALL)
    {
        QModelIndex indexCall1 = sourceModel()->index(sourceRow, FT8DemodSettings::MESSAGE_COL_CALL1, sourceParent);
        QModelIndex indexCall2 = sourceModel()->index(sourceRow, FT8DemodSettings::MESSAGE_COL_CALL2, sourceParent);
        return (sourceModel()->data(indexCall1).toString() == m_call) ||
            (sourceModel()->data(indexCall2).toString() == m_call);
    }

    if (m_filterActive == FILTER_LOC)
    {
        QModelIndex index = sourceModel()->index(sourceRow, FT8DemodSettings::MESSAGE_COL_LOC, sourceParent);
        return sourceModel()->data(index).toString() == m_loc;
    }

    if (m_filterActive == FILTER_COUNTRY)
    {
        QModelIndex index = sourceModel()->index(sourceRow, FT8DemodSettings::MESSAGE_COL_COUNTRY, sourceParent);
        return sourceModel()->data(index).toString() == m_country;
    }

    if (m_filterActive == FILTER_INFO)
    {
        QModelIndex index = sourceModel()->index(sourceRow, FT8DemodSettings::MESSAGE_COL_INFO, sourceParent);
        const QString& content = sourceModel()->data(index).toString();

        if (m_info.startsWith("OSD")) {
            return content.startsWith("OSD");
        } else {
            return !content.startsWith("OSD");
        }
    }

    return true;
}
