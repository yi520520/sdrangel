///////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021 Jon Beniston, M7RCE <jon@beniston.com>                         //
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
#include "DVBS2.h"
//
// The output is bit packed ready for modulating
//
void DVBS2::s2_interleave( void )
{
    int index=0;
    int rows=0;

    int frame_size = m_format[0].nldpc;

    if( m_format[0].constellation == M_QPSK )
    {
        rows = frame_size/2;
        m_payload_symbols =  rows;
        for( int i = 0; i < rows; i++ )
        {
            m_iframe[i]  = (m_frame[index++]<<1);
            m_iframe[i] |= m_frame[index++];
        }
        return;
    }

    if( m_format[0].constellation == M_8PSK )
    {
        if( m_format[0].code_rate == CR_3_5 )
        {
            rows = frame_size/3;
            m_payload_symbols =  rows;
            Bit *c1,*c2,*c3;
            c1 = &m_frame[rows*2];
            c2 = &m_frame[rows];
            c3 = &m_frame[0];
            for( int i = 0; i < rows; i++ )
            {
                m_iframe[i]  = (c1[i]<<2) | (c2[i]<<1) | (c3[i]);
            }
        }
        else
        {
            rows = frame_size/3;
            m_payload_symbols =  rows;
            Bit *c1,*c2,*c3;
            c1 = &m_frame[0];
            c2 = &m_frame[rows];
            c3 = &m_frame[rows*2];
            for( int i = 0; i < rows; i++ )
            {
                m_iframe[i]  = (c1[i]<<2) | (c2[i]<<1) | (c3[i]);
            }
        }
        return;
    }

    if( m_format[0].constellation == M_16APSK )
    {
        rows = frame_size/4;
        m_payload_symbols =  rows;
        Bit *c1,*c2,*c3,*c4;
        c1 = &m_frame[0];
        c2 = &m_frame[rows];
        c3 = &m_frame[rows*2];
        c4 = &m_frame[rows*3];
        for( int i = 0; i < rows; i++ )
        {
            m_iframe[i]  = (c1[i]<<3) | (c2[i]<<2) | (c3[i]<<1) | (c4[i]);
        }
        return;
    }

    if( m_format[0].constellation == M_32APSK )
    {
        rows = frame_size/5;
        m_payload_symbols =  rows;
        Bit *c1,*c2,*c3,*c4,*c5;
        c1 = &m_frame[0];
        c2 = &m_frame[rows];
        c3 = &m_frame[rows*2];
        c4 = &m_frame[rows*3];
        c5 = &m_frame[rows*4];
        for( int i = 0; i < rows; i++ )
        {
            m_iframe[i]  = (c1[i]<<4) | (c2[i]<<3) | (c3[i]<<2) | (c4[i]<<1) | c5[i];
        }

        return;
    }
}
