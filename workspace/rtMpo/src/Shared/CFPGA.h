/*
 * CFPGA.h
 *
 *  Created on: Feb 12, 2021
 *      Author: anton
 */
#include <Mfbsp/CGPIO.h>
#include "../System/MMIProto.h"

#ifndef SRC_SHARED_CFPGA_H_
#define SRC_SHARED_CFPGA_H_

class CFPGA {
public:
	//
	// FPGA Receiver parameters.
	//
	typedef struct {
		u_int reset_dsp;		// Сброс приемника

		u_int adc_atten_1;		// Управление аттенюатором 1 и 2
		u_int adc_atten_2 ;

		int freq_ddc_nco_1;			// Частота первого переноса частоты в DDC
		int freq_ddc_nco_2[4];		// Частоты второго переноса частоты в DDC

		u_int corr_lvl_coef;		// Множитель при вычислении уровня в корреляции

		u_int peak_detect_sig_del;	// Задержка сигнала в пиковом детекторе
		u_int peak_detect_corr_del;	// Задержка корреляции в пиковом детекторе


		u_int ddc_filter_coefs[436];	//массив с коэффициентами ddc фильтра
		u_int corr_coefs_real[256];		//массив с коэффициентами корреляционного фильтра (действительная часть)
		u_int corr_coefs_imag[256];		//массив с коэффициентами корреляционного фильтра (мнимая часть)
		u_int recv_filter_coefs[129];	//массив с коэффициентами приемного фильтра
	} recv_coefs_var_t ;

	//
	// FPGA LPort parameters
	//
	typedef struct {
		u_int en;
		u_int mod_trig;
		u_int src;
		u_int chan;
	} recv_lport_var_t ;

private:
	CGPIO *pGPIO ;

	//
	// Default values for FPGA firmware:
	//
	static double initCorrLvlCoef_f ;
	static double initFreqSampl0 ;
	static double initFreqChannel[4] ;

	static u_int defaultResetDSP ;
	static u_int defaultADCAtten_1 ;
	static u_int defaultADCAtten_2 ;
	static u_int defaultPeakDetectSigDel ;
	static u_int defaultPeakDetectCorrDel ;
	static u_int defaultDDCFilterCoef[436] ;
	static u_int defaultCorrCoefsReal[256] ;
	static u_int defaultCorrCoefsImag[256] ;
	static u_int defaultRecvFilterCoef[129] ;

	int defaultFreqDDC_NCO_1 ;
	int defaultFreqDDC_NCO_2[4] ;
	u_int defaultCorrLvlCoef ;

	recv_lport_var_t	defaultLPortParams[2] ;

	static u_int setupBaseAddr ;

	virtual int initDefault() ;

public:
	CFPGA();
	virtual ~CFPGA();
	virtual int startBoot(int PROM) ;
	virtual bool isBooted() ;
	virtual int getDefaultReceiverCoeffs(recv_coefs_var_t &coefs) ;
	virtual int writeReg32(u_int regNo, u_int data) ;
	virtual int	applyReceiverCoeffs(const recv_coefs_var_t &coefs) ;
	virtual int	getDefaultLPortParams(recv_lport_var_t (&lport)[2]) ;
	virtual int	applyLPortParams(const recv_lport_var_t (&lport)[2]) ;
	static void printCoefs(const recv_coefs_var_t &coefs) ;
	static void	printLPortParams(const recv_lport_var_t (&lport)[2]) ;

	static int dbl2fix(double num, u_char fractBits ) ;
	static double fract1(double num) ;
	void calcFreqDDC_NCO(
									double freqSampl0,
									double const (&freqChannel)[4],
									int &freqDDC_NCO_1,
									int (&freqDDC_NCO_2)[4]
								) ;
	void parseMMISetModeCmdArgs(
									mmi_cmd_mpo_setmode_t &cmd,
									recv_coefs_var_t coefs,
									recv_lport_var_t (&lport)[2]
								) ;
	static double getFreqMin(double const (&freq)[4]) ;
	static double getFreqMax(double const (&freq)[4]) ;

	double getCurrentFreqChannel(u_int idx) ;
};

#endif /* SRC_SHARED_CFPGA_H_ */
