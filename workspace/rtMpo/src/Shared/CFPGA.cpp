/*
 * CFPGA.cpp
 *
 *  Created on: Feb 12, 2021
 *      Author: anton
 */

#if 0
	#define DEBUG
#endif

#if 1
	#define VERBOSE
#endif

#include "CFPGA.h"
#include <auxtimer.h>
#include <auxio.h>
#include <math.h>
#include <auxmath.h>

#define BOOT_FPGA_GPIO_DEV_PATH		"/dev/gpio3"

#define GPIO_FPGA_DONE		( 1 << 1 )
#define GPIO_OE_PROM(no)	(1 << (2 + no * 2))
#define GPIO_nCE_PROM(no)	(1 << (3 + no * 2))
#define GPIO_FPGA_PROG_B	(1 << 6)
#define GPIO_FPGA_INIT_B	(1 << 7)

#define FPGA_REG_RESET_DSP				0
#define FPGA_REG_FILTER_COEF_L			2
#define FPGA_REG_FILTER_COEF_H			3
#define FPGA_REG_CORR_COEF_REAL_L		2
#define FPGA_REG_CORR_COEF_REAL_H		3
#define FPGA_REG_CORR_COEF_IMAG_L		2
#define FPGA_REG_CORR_COEF_IMAG_H		3
#define FPGA_REG_RECV_FILTER_COEF_L		2
#define FPGA_REG_RECV_FILTER_COEF_H		3
#define FPGA_REG_WRITE_ADDR				4
#define FPGA_REG_WRITE_EN				1
#define FPGA_REG_ADC_ATTEN				5
#define FPGA_REG_FREQ_DDC_NCO_1_L		6
#define FPGA_REG_FREQ_DDC_NCO_1_H		7
#define FPGA_REG_FREQ_DDC_NCO_2_0		8
#define FPGA_REG_CORR_LVL_COEF			16
#define FPGA_REG_PEAK_DETECT_SIG_DEL	17
#define FPGA_REG_PEAK_DETECT_CORR_DEL	18
#define FPGA_REG_LPORT_0				19
#define FPGA_REG_LPORT_1				20

// Base address for FPGA receiver setup.
u_int CFPGA::setupBaseAddr = 0xA1000000 ;

typedef struct {
	uint32_t	chan: 	8 ;
	uint32_t	src:	2 ;
	uint32_t	trig:	1 ;
	uint32_t	en:		1 ;
} fpga_lport_reg_t;

CFPGA::CFPGA() {
	//
	// Create new GPIO object instance for FPGA booting.
	//
	pGPIO = new CGPIO(BOOT_FPGA_GPIO_DEV_PATH, O_RDWR) ;

	//
	// Calculate some default FPGA receiver coefficients
	//
	initDefault() ;

}

CFPGA::~CFPGA() {

	//
	// Destroy GPIO object.
	//
	delete pGPIO ;
}

/*
 * FPGA booting. FPGA has two PROMs? and we can select
 * which one should be loaded by argument <int PROM>.
 */
int CFPGA::startBoot(int PROM) {

	//
	// Set up GPIO direction:
	//
	// FPGA_DONE	(LCLK3)		- вход
	// OE_PROM_0	(LDAT3[0])	- выход
	// nCE_PROM_0	(LDAT3[1])	- выход
	// OE_PROM_1	(LDAT3[2])	- выход
	// nCE_PROM_1	(LDAT3[3])	- выход
	// FPGA_PROG_B	(LDAT3[4]) 	- выход
	// FPGA_INIT_B	(LDAT3[5]) 	- вход
	//
	uint16_t dir = GPIO_OE_PROM(0) | GPIO_OE_PROM(1) | GPIO_nCE_PROM(0)
			| GPIO_nCE_PROM(1) | GPIO_FPGA_PROG_B ;

	DTRACE("set(dir=0x%x)\n", dir) ;
	pGPIO->setDirection(dir) ;

	//
	// Prepare for FPGA booting.
	//
	uint16_t data = 0x0 ;
	pGPIO->get(data) ;
	DTRACE("get(data=0x%x)\n", data) ;

	for (int i = 0; i < 2; i++) {
		data &= ~GPIO_OE_PROM(i); 			// clear OE_PROM_i
		data |= GPIO_nCE_PROM(i); 			// set nCE_PROM_i
	}
	data &= ~GPIO_FPGA_PROG_B ;				// clear FPGA_PROG_B
	DTRACE("set(data=0x%x)\n", data) ;
	pGPIO->set( data ) ;

	//
	// Select particular PROM for booting.
	//
	data |= GPIO_OE_PROM(PROM) ; 			// set OE_PROM_0
	data &= ~GPIO_nCE_PROM(PROM);			// clear nCE_PROM_0
	DTRACE("set(data=0x%x)\n", data) ;
	pGPIO->set( data ) ;

	//
	// Start booting. Boot process will continue during maximum 1 second.
	// So we have to check if it will boot later.
	//
	data |= GPIO_FPGA_PROG_B ;				// set FPGA_PROG_B
	DTRACE("set(data=0x%x)\n", data) ;
	pGPIO->set( data ) ;

	pGPIO->get(data) ;
	DTRACE("get(data=0x%x)\n", data) ;

	//
	// If initialization process didn't start,
	// report an error.
	//
	if (data & GPIO_FPGA_INIT_B) {

		return -1 ;
	}

	return 0 ;
}

bool CFPGA::isBooted() {
	uint16_t data ;
	bool done = false ;

#ifdef DEBUG
	uint16_t dir ;
	//
	//	Get current GPIO direction
	//
	pGPIO->getDirection(dir) ;
	TRACE("get(dir=0x%x)\n", dir) ;
#endif

	pGPIO->get(data) ;
	DTRACE("get(data=0x%x)\n", data) ;

	//
	// If CRC check after initialization is OK,
	// the we assume that boot process is done.
	//
	if ( (data & GPIO_FPGA_INIT_B) && (data & GPIO_FPGA_DONE) ) {

		done = true ;
	}

	DTRACE("RETURN %i\n", done) ;
	return done ;
}

/*
 * Converts floating point number to fixed point number.
 */
int CFPGA::dbl2fix(double num, u_char fractBits) {

	return (num * (1 << fractBits)) ;
}

/*
 * Returns fractional part of number. if number is negative, then
 * returns (1-fractional).
 */
double CFPGA::fract1(double num) {
	int intNum = num ;

	double res = (num - intNum) ;

	return res > 0 ? res : (res + 1) ;
}

void CFPGA::calcFreqDDC_NCO(	double freqSampl0,
								double const (&freqChannel)[4],
								int &freqDDC_NCO_1,
								int (&freqDDC_NCO_2)[4]) 	{

	// Средняя отрицательная частота для крайних каналов
	double freqDDC_Avg = -(getFreqMin(freqChannel) + getFreqMax(freqChannel))/2 ;

	// Новые несущие частоты
	double freqDDC_NCO_2_f[4] ;
	for (u_int i= 0; i < ARRLEN(freqChannel); i++) {
		freqDDC_NCO_2_f[i] = freqChannel[i] + freqDDC_Avg ;
	}

	// Частоты второго переноса
	for (u_int i = 0; i < ARRLEN(freqChannel); i++) {
		freqDDC_NCO_2_f[i] *= -1 ;
	}

	// Hz -> MHz
	freqSampl0 /= 1e6 ;
	// Частота дискретизации при втором переносе
	double freqSampl2 = freqSampl0 / 4.0 ;

	// Нормированные цифровые частоты
	double freqDDC_NCO_1_f = freqDDC_Avg / freqSampl0 ;
	for (u_int i = 0; i < ARRLEN(freqChannel); i++) {
		freqDDC_NCO_2_f[i] /= freqSampl2 ;
	}

	// С фиксированной точкой
	freqDDC_NCO_1 = dbl2fix(fract1(freqDDC_NCO_1_f), 24) ;
	for (u_int i = 0; i < ARRLEN(freqDDC_NCO_2); i++) {
		freqDDC_NCO_2[i] = dbl2fix(fract1(freqDDC_NCO_2_f[i]), 24) ;
	}
}

int CFPGA::initDefault() {
	double defaultFreqDDC_NCO_1_f ;
	double defaultFreqDDC_NCO_2_f[4] ;

	//
	// Initialize default values for FPGA receiver.
	//
	/*calcFreqNco_f(initFreqSampl0, initFreqChannel, 4, &defaultFreqDDC_NCO_1_f, &defaultFreqDDC_NCO_2_f[0]);

	defaultFreqDDC_NCO_1 = dbl2fix(ABS(fract1(defaultFreqDDC_NCO_1_f)), 24);
	for (int i = 0; i < 4; i++) {
		defaultFreqDDC_NCO_2[i]	= dbl2fix(ABS(fract1(defaultFreqDDC_NCO_2_f[i])), 24);
	}*/

	/*defaultFreqDDC_NCO_1 = 4536548 ;
	defaultFreqDDC_NCO_2[0] = 1911467 ;
	defaultFreqDDC_NCO_2[1] = 1875058 ;
	defaultFreqDDC_NCO_2[2] = 14902158 ;
	defaultFreqDDC_NCO_2[3] = 14865749 ;*/

	calcFreqDDC_NCO(initFreqSampl0, initFreqChannel, defaultFreqDDC_NCO_1, defaultFreqDDC_NCO_2) ;

	defaultCorrLvlCoef = dbl2fix(initCorrLvlCoef_f, 15) ;

	//
	// Initialize default values for FPGA lport.
	//
	defaultLPortParams[0].en = 1 ;
	defaultLPortParams[0].mod_trig = 0 ;
	//defaultLPortParams[0].src = 2 ;			// ADC, token=0xab55
	//defaultLPortParams[0].src = 1 ; 		// DDC (СВС), token=0xab55
	defaultLPortParams[0].src = 0 ; 		// Inf (СВИ), token=0xaa55
	//defaultLPortParams[0].src = 3 ;			// counter, token=0xab55
	defaultLPortParams[0].chan = 255 ;
	//defaultLPortParams[0].chan = 1 ;

	defaultLPortParams[1].en = 1 ;
	//defaultLPortParams[1].mod_trig = 1 ;
	defaultLPortParams[1].mod_trig = 0 ;
	//defaultLPortParams[1].src = 2 ;			// ADC, token=0xab55
	//defaultLPortParams[1].src = 1 ;		// DDC (СВС), token=0xab55
	defaultLPortParams[1].src = 0 ;			// Inf (СВИ), token=0xaa55
	//defaultLPortParams[1].src = 3 ;			// counter, token=0xab55
	defaultLPortParams[1].chan = 255 ;
	//defaultLPortParams[1].chan = 1 ;

	return 0 ;
}

int CFPGA::getDefaultReceiverCoeffs(CFPGA::recv_coefs_var_t &coefs) {

	coefs.reset_dsp = defaultResetDSP ;

	coefs.adc_atten_1 = defaultADCAtten_1 ;
	coefs.adc_atten_2 = defaultADCAtten_2 ;

	for(int i = 0; i < 256; i++) {
		coefs.corr_coefs_real[i] = defaultCorrCoefsReal[i] ;
		coefs.corr_coefs_imag[i] = defaultCorrCoefsImag[i] ;
	}

	coefs.corr_lvl_coef = defaultCorrLvlCoef ;

	for (int i = 0; i < 436; i++) {
		coefs.ddc_filter_coefs[i] = defaultDDCFilterCoef[i] ;
	}

	coefs.freq_ddc_nco_1 = defaultFreqDDC_NCO_1 ;
	for (int i = 0; i < 4; i++) {
		coefs.freq_ddc_nco_2[i] = defaultFreqDDC_NCO_2[i] ;
	}

	for (int i = 0; i < 129; i++) {
		coefs.recv_filter_coefs[i] = defaultRecvFilterCoef[i] ;
	}

	coefs.peak_detect_corr_del = defaultPeakDetectCorrDel ;
	coefs.peak_detect_sig_del = defaultPeakDetectSigDel ;

	return 0 ;
}

int CFPGA::writeReg32(u_int regNo, u_int data) {

	size_t dataSize = sizeof(data);

#if 0
	//DTRACE("*0x%X=%U\n", setupBaseAddr + (regNo * dataSize), data) ;
	DTRACE("*0x%X(%d)=0x%04X\n", ((uint32_t*)setupBaseAddr + regNo), regNo, data) ;
#endif
	/*for (size_t i = 0; i < dataSize; i++) {
		*((u_char*)(setupBaseAddr + (regNo * dataSize) + i)) = (data >> 8*i) & 0xFF ;
	}*/
	*((uint32_t*)setupBaseAddr + regNo) = data ;

	return 0 ;
}

int CFPGA::applyReceiverCoeffs(const CFPGA::recv_coefs_var_t &coefs) {

	writeReg32(FPGA_REG_RESET_DSP, coefs.reset_dsp) ;

	//
	// Setup DDC filter coefficients
	//
	for (size_t i = 0; i < ARRLEN(coefs.ddc_filter_coefs); i++) {
		writeReg32(FPGA_REG_FILTER_COEF_L, coefs.ddc_filter_coefs[i] & 0xFFFF) ;
		writeReg32(FPGA_REG_FILTER_COEF_H, (coefs.ddc_filter_coefs[i] >> 16) & 0xFFFF) ;
		writeReg32(FPGA_REG_WRITE_ADDR, i) ;
		writeReg32(FPGA_REG_WRITE_EN, 0x8) ;
		writeReg32(FPGA_REG_WRITE_EN, 0x0) ;
	}

	//
	//	Setup real part of correlation coefficients
	//
	for (size_t i = 0; i < ARRLEN(coefs.corr_coefs_real); i++) {
		writeReg32(FPGA_REG_CORR_COEF_REAL_L, coefs.corr_coefs_real[i] & 0xFFFF) ;
		writeReg32(FPGA_REG_CORR_COEF_REAL_H, (coefs.corr_coefs_real[i] >> 16) & 0xFFFF) ;
		writeReg32(FPGA_REG_WRITE_ADDR, i) ;
		writeReg32(FPGA_REG_WRITE_EN, 0x4) ;
		writeReg32(FPGA_REG_WRITE_EN, 0x0) ;
	}

	//
	// Setup imaginary part of correlation coefficients.
	//
	for (size_t i = 0; i < ARRLEN(coefs.corr_coefs_imag); i++) {
		writeReg32(FPGA_REG_CORR_COEF_IMAG_L, coefs.corr_coefs_imag[i] & 0xFFFF) ;
		writeReg32(FPGA_REG_CORR_COEF_IMAG_H, (coefs.corr_coefs_imag[i] >> 16) & 0xFFFF) ;
		writeReg32(FPGA_REG_WRITE_ADDR, i) ;
		writeReg32(FPGA_REG_WRITE_EN, 0x2) ;
		writeReg32(FPGA_REG_WRITE_EN, 0x0) ;
	}

	//
	// Setup receiver's filter coefficients.
	//
	for (size_t i = 0; i < ARRLEN(coefs.recv_filter_coefs); i++) {
		writeReg32(FPGA_REG_RECV_FILTER_COEF_L, coefs.recv_filter_coefs[i] & 0xFFFF) ;
		writeReg32(FPGA_REG_RECV_FILTER_COEF_H, (coefs.recv_filter_coefs[i] >> 16) & 0xFFFF) ;
		writeReg32(FPGA_REG_WRITE_ADDR, i) ;
		writeReg32(FPGA_REG_WRITE_EN, 0x1) ;
		writeReg32(FPGA_REG_WRITE_EN, 0x0) ;
	}

	writeReg32(1, 0x0) ;
	writeReg32(2, 0x0) ;
	writeReg32(3, 0x0) ;
	writeReg32(4, 0x0) ;

	writeReg32(FPGA_REG_ADC_ATTEN, ((coefs.adc_atten_1 & 0x3F) << 6) | (coefs.adc_atten_2 & 0x3F) ) ;

	writeReg32(FPGA_REG_FREQ_DDC_NCO_1_L, coefs.freq_ddc_nco_1 & 0xFFFF) ;
	writeReg32(FPGA_REG_FREQ_DDC_NCO_1_H, (coefs.freq_ddc_nco_1 >> 16) & 0xFFFF) ;

	for (size_t i = 0; i < ARRLEN(coefs.freq_ddc_nco_2); i++) {
		writeReg32(FPGA_REG_FREQ_DDC_NCO_2_0 + 2*i, coefs.freq_ddc_nco_2[i] & 0xFFFF) ;
		writeReg32(FPGA_REG_FREQ_DDC_NCO_2_0 + 2*i + 1, (coefs.freq_ddc_nco_2[i] >> 16) & 0xFFFF) ;
	}

	writeReg32(FPGA_REG_CORR_LVL_COEF, coefs.corr_lvl_coef) ;

	writeReg32(FPGA_REG_PEAK_DETECT_SIG_DEL, coefs.peak_detect_sig_del) ;

	writeReg32(FPGA_REG_PEAK_DETECT_CORR_DEL, coefs.peak_detect_corr_del) ;

	if (coefs.reset_dsp) {
		writeReg32(FPGA_REG_RESET_DSP, 0) ;
	}

	return 0 ;
}

int CFPGA::getDefaultLPortParams(recv_lport_var_t (&lport)[2]) {

	for (int i = 0; i < 2; i++) {

		lport[i] = defaultLPortParams[i] ;
	}

	return 0 ;
}

int	CFPGA::applyLPortParams(const recv_lport_var_t (&lport)[2]) {

	writeReg32(FPGA_REG_LPORT_0, (lport[0].en << 11) | (lport[0].mod_trig << 10)
				| (lport[0].src << 8) | lport[0].chan) ;

	writeReg32(FPGA_REG_LPORT_1, (lport[1].en << 11) | (lport[1].mod_trig << 10)
				| (lport[1].src << 8) | lport[1].chan) ;

	return 0 ;
}

void CFPGA::printLPortParams(const recv_lport_var_t (&lport)[2]) {
	for (u_int i = 0; i < ARRLEN(lport); i++ ) {
		printf("lport%i: en=%i, src=%i, mod_trig=%i, chan=%i\n",
				i, lport[i].en, lport[i].src, lport[i].mod_trig, lport[i].chan) ;
	}
	fflush(stdout) ;
}

void CFPGA::printCoefs(const CFPGA::recv_coefs_var_t &coefs) {
	printf("reset_dsp=%u\n", coefs.reset_dsp) ;

	printf("adc_atten_1=%u\n", coefs.adc_atten_1) ;
	printf("adc_atten_2=%u\n", coefs.adc_atten_2) ;

	printf("freq_ddc_nco_1=%u\n", coefs.freq_ddc_nco_1) ;

	printf("freq_ddc_nco_2[:%u]= ", ARRLEN(coefs.freq_ddc_nco_2)) ;
	for(int i = 0; i < 4; i++)  printf("%u ", coefs.freq_ddc_nco_2[i]) ;
	printf("\n") ;

	printf("corr_lvl_coef=%u\n", coefs.corr_lvl_coef) ;

	printf("peak_detect_sig_del=%u\n", coefs.peak_detect_sig_del) ;
	printf("peak_detect_corr_del=%u\n", coefs.peak_detect_corr_del) ;

	printf("ddc_filter_coefs[:%u]= ", ARRLEN(coefs.ddc_filter_coefs)) ;
	for(int i = 0; i < 7; i++) printf("%u ", coefs.ddc_filter_coefs[i]) ;
	printf("\n") ;

	printf("corr_coefs_real[:%u]= ", ARRLEN(coefs.corr_coefs_real)) ;
	for(int i = 0; i < 8; i++)	printf("%u ", coefs.corr_coefs_real[i]) ;
	printf("\n") ;

	printf("corr_coefs_imag[:%u]= ", ARRLEN(coefs.corr_coefs_imag)) ;
	for(int i = 0; i < 8; i++)	printf("%u ", coefs.corr_coefs_imag[i]) ;
	printf("\n") ;

	printf("recv_filter_coefs[:%u]= ", ARRLEN(coefs.recv_filter_coefs)) ;
	for (int i = 0 ; i < 7; i++) printf("%u ", coefs.recv_filter_coefs[i]) ; ;
	printf("\n") ;
}

void CFPGA::parseMMISetModeCmdArgs(
							mmi_cmd_mpo_setmode_t &cmd,
							CFPGA::recv_coefs_var_t coefs,
							CFPGA::recv_lport_var_t (&lport)[2]
						) {

	VPRINTF("FPGA: parsing ARG[:9]={") ;
	for (u_int i = 0; i < 9; i++) {
		VPRINTF("0x%X ", cmd.arg[i]) ;
	}
	VPRINTF("}\n") ;

	if (cmd.arg[0] == 1) {  //СВИАИС (декодированные сообщения)
		lport[0].src = 0 ;
		lport[1].src = 0 ;
		if (cmd.arg[4] == 1) {	//  Номер ВЧ тракта: ВК1
			lport[0].en = 1 ;
			lport[1].en = 0 ;
			lport[0].chan= cmd.arg[1] ;
		}
		if (cmd.arg[4] == 2) {  //  Номер ВЧ тракта: ВК2
			//lport[0].en = 0 ;
			lport[0].en = 1 ;
			//lport[1].en = 1 ;
			//lport[1].chan = cmd.arg[1] << 4 ;
			lport[0].chan = cmd.arg[1] << 4 ;
		}
		if (cmd.arg[4] == 3) {	//  Номер ВЧ тракта: ВК1+ВК2
			lport[0].en = 1 ;
			//lport[1].en = 1 ;
			lport[1].en = 0 ;
			lport[0].chan = cmd.arg[1] | cmd.arg[1] << 4 ;
			lport[1].chan = (cmd.arg[1]<<4) ;
			/* если в arg[1] лежит например 00000000000001102 то в
			l_port2_chan должно оказаться 011000002 */
		}
	}

	if (cmd.arg[0] == 0){ //СВСАИС (оцифрованный сигнал)
		lport[0].src = 1 ;
		lport[1].src = 1 ;

		// Если arg[7] == 1:
		// используем альтернативные (загруженные в полете) настройки (массивы коэффициентов),
		// если не удалось загрузить их из памяти, то используем настройки по-умолчанию, и сообщаем МЗУ об этом в телеметрии.
		//
		//рассчитать частоту второго переноса на основе arg[5-6] и передать ее вместо первого элемента массива Freq_nco_2
		/* заменить значение 1911467  на рассчитанное */
		double newFreqChannel[4] ;

		newFreqChannel[0] = (cmd.arg[6] << 16 | cmd.arg[5]) / 1e6 ;
		for (u_int i = 1; i < ARRLEN(newFreqChannel); i++) {
			newFreqChannel[i] = initFreqChannel[i] ;
		}
		int freqDDC_NCO_1 ;
		int freqDDC_NCO_2[4] ;
		calcFreqDDC_NCO(initFreqSampl0, newFreqChannel, freqDDC_NCO_1, freqDDC_NCO_2) ;
		coefs.freq_ddc_nco_2[0] = freqDDC_NCO_2[0] ;

		if (cmd.arg[4] == 1){ 	//ВК1
			lport[0].en = 1 ;
			lport[1].en = 0 ;
			lport[0].chan = 0x1 ;
		}
		if (cmd.arg[4] == 2){ 	//ВК2
			lport[0].en = 0 ;
			lport[1].en = 1 ;
			lport[1].chan = 0x10 ;
		}
		if (cmd.arg[4] == 3){ 	//ВК1+ВК2
			lport[0].en = 1 ;
			lport[1].en = 1 ;
			lport[0].chan = 0x1 ;
			lport[1].chan = 0x10 ;
		}
	}

	coefs.adc_atten_1 = cmd.arg[2] ;
	coefs.adc_atten_2 = cmd.arg[3] ;

	DTRACE("lport[0].src=%i, lport[1].src=%i\n", lport[0].src, lport[1].src) ;
}

double CFPGA::getFreqMin(double const (&freq)[4]) {
	double min = freq[0] ;

	for (u_int i = 1; i < ARRLEN(freq); i++) {
		min = (min < freq[i]) ? min : freq[i] ;
	}

	return min ;
}

double CFPGA::getFreqMax(double const (&freq)[4]) {
	double max = freq[0] ;

	for (u_int i = 1; i < ARRLEN(freq); i++) {
		max = (max > freq[i]) ? max : freq[i] ;
	}

	return max ;
}
