///////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany     //
// written by Christian Daniel                                                       //
// Copyright (C) 2014-2015 John Greb <hexameron@spam.no>                             //
// Copyright (C) 2015-2018, 2022-2023 Edouard Griffiths, F4EXB <f4exb06@gmail.com>   //
// Copyright (C) 2020 Kacper Michajłow <kasper93@gmail.com>                          //
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
/*
 * Filters from Fldigi.
*/

#ifndef	_FFTFILT_H
#define	_FFTFILT_H

#include <complex>
#include <cmath>

#include "gfft.h"
#include "fftwindow.h"
#include "fftnr.h"
#include "export.h"

//----------------------------------------------------------------------

class SDRBASE_API fftfilt {
enum {NONE, BLACKMAN, HAMMING, HANNING};

public:
	typedef std::complex<float> cmplx;

	fftfilt(int len);
	fftfilt(float f1, float f2, int len);
	fftfilt(float f2, int len);
	~fftfilt();
// f1 < f2 ==> bandpass
// f1 > f2 ==> band reject
	void create_filter(float f1, float f2, FFTWindow::Function wf = FFTWindow::Blackman);
    void create_filter(const std::vector<std::pair<float, float>>& limits, bool pass = true, FFTWindow::Function wf = FFTWindow::Blackman);
    void create_filter(const std::vector<std::pair<float, float>>& limits, bool pass = true); //!< Windowless version
	void create_dsb_filter(float f2, FFTWindow::Function wf = FFTWindow::Blackman);
    void create_asym_filter(float fopp, float fin, FFTWindow::Function wf = FFTWindow::Blackman); //!< two different filters for in band and opposite band
    void create_rrc_filter(float fb, float a); //!< root raised cosine. fb is half the band pass

	int noFilt(const cmplx& in, cmplx **out);
	int runFilt(const cmplx& in, cmplx **out);
	int runSSB(const cmplx& in, cmplx **out, bool usb, bool getDC = true);
	int runDSB(const cmplx& in, cmplx **out, bool getDC = true);
	int runAsym(const cmplx & in, cmplx **out, bool usb); //!< Asymmetrical filtering can be used for vestigial sideband

    void setDNR(bool dnr) { m_dnr = dnr; }
    void setDNRScheme(FFTNoiseReduction::Scheme scheme) { m_dnrScheme = scheme; }
    void setDNRAboveAvgFactor(float aboveAvgFactor) { m_dnrAboveAvgFactor = aboveAvgFactor; }
    void setDNRSigmaFactor(float sigmaFactor) { m_dnrSigmaFactor = sigmaFactor; }
    void setDNRNbPeaks(int nbPeaks) { m_dnrNbPeaks = nbPeaks; }
    void setDNRAlpha(float alpha) { m_noiseReduction.setAlpha(alpha); }

protected:
    // helper class for FFT based noise reduction
	int flen;
	int flen2;
	g_fft<float> *fft;
	cmplx *filter;
    cmplx *filterOpp;
	cmplx *data;
	cmplx *ovlbuf;
	cmplx *output;
	int inptr;
	int pass;
	int window;
    bool m_dnr;
    FFTNoiseReduction::Scheme m_dnrScheme;
    float m_dnrAboveAvgFactor; //!< above average factor
    float m_dnrSigmaFactor; //!< sigma multiplicator for average + std deviation
    int m_dnrNbPeaks; //!< number of peaks (peaks scheme)
    FFTNoiseReduction m_noiseReduction;

	inline float fsinc(float fc, int i, int len)
	{
	    int len2 = len/2;
		return (i == len2) ? 2.0 * fc:
				sin(2 * M_PI * fc * (i - len2)) / (M_PI * (i - len2));
	}

	inline float _blackman(int i, int len)
	{
		return (0.42 -
				 0.50 * cos(2.0 * M_PI * i / len) +
				 0.08 * cos(4.0 * M_PI * i / len));
	}

	/** RRC function in the frequency domain. Zero frequency is on the sides with first half in positive frequencies
	 * and second half in negative frequencies */
	inline cmplx frrc(float fb, float a, int i, int len)
	{
        float x = i/(float)len; // normalize to [0..1]
        x = 0.5-fabs(x-0.5); // apply symmetry: now both halves overlap near 0
        float tr = fb*a; // half the transition zone

        if (x < fb-tr)
        {
            return 1.0; // in band
        }
        else if (x < fb+tr) // transition
        {
            float y = ((x-(fb-tr)) / (2.0*tr))*M_PI;
            return (cos(y) + 1.0f)/2.0f;
        }
        else
        {
            return 0.0; // out of band
        }
	}

	void init_filter();
	void init_dsb_filter();
};



/* Sliding FFT filter from Fldigi */
class SDRBASE_API sfft {
#define K1 0.99999
public:
	typedef std::complex<float> cmplx;
	sfft(int len);
	~sfft();
	void run(const cmplx& input);
	void fetch(float *result);
private:
	int fftlen;
	int first;
	int last;
	int ptr;
	struct vrot_bins_pair;
	vrot_bins_pair *vrot_bins;
	cmplx *delay;
	float k2;
};

#endif
