/*
 *  calctime1.c
 *
 *  This module demonstrates the POSIX clock,
 *  and also shows the ClockCycles call
 *
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __QNX__
#   include <sys/neutrino.h>
#   include <sys/syspage.h>
#endif
#include <pthread.h>

#include <math.h>
#include <time.h>


#define BILLION         1000000000
#define MILLION         1000000
#define NumSamples      30



struct OrbParam_var {
	double x;
	double y;
	double z;
	double inc;
	double vx;
	double vy;
	double vz;
	double t0;
	double t;
	double dt;
	double Sm;
	double Cx;
	double m;
	double Fs;
	double oms;
	double ps;
	double es;
	double bigoms;
	double incs;
	double omm;
	double pm;
	double em;
	double bigomm;
	double incm;
	int hour;
	int min;
	int sec;
	int day;
	int month;
	int year;
	double Angle_pitch;
	double Angle_roving;
	double Angle_bank;
	double omega_x;
	double omega_y;
	double omega_z;
	double latitude;
};
struct OrbParam {
	int init;
	struct OrbParam_var Param;
	double u1s;
	double u1m;
	double a0;
	double a1;
	double a2;
	double a3;
	double as;
	double M0s;
	double am;
	double M0m;
};
struct OrbParam data;
int init = 1;
char *progname = "calctime1";

int main()
{
	struct timespec clockval, prevclockval;
	uint64_t delta, cyclespersec, cyclespermicrosec, cyclespernanosec;
	uint64_t cycs[NumSamples];
	int i;
#ifdef __QNX__
	time_t t1, t2;
    uint64_t aa,bb;
#endif
	int main_counter;

	printf("\nClockTime deltas:\n");

	/*
	 *  capture some clock changes
	 */
#ifdef __QNX__
	struct _clockperiod newval, oldval;
	newval.nsec = 1000*10; // between 100 mks and 1 ms
	newval.fract = 0;	// femta secs, so always 0
	ClockPeriod (CLOCK_REALTIME, &newval, &oldval, 0);
#endif


	/*
	 *  next, we very quickly snapshot some values of the ClockCycles
	 *  call.  We don't do any printf's, as we want to see the speed
	 *  with which the "for" loop executes.
	 */
	data.init = init;
	data.Param.x = 6778160;
	data.Param.y = 0;
	data.Param.z = 0;
	data.Param.inc = 0;
	data.Param.vx = 0;
	data.Param.vy = 7666.088;
	data.Param.vz = 0;
	data.Param.t0 = 0;
	data.Param.t = 0;
	data.Param.dt = 1;
	data.Param.Sm = 1;
	data.Param.Cx = 1;
	data.Param.m = 350;
	data.Param.Fs = 80;
	data.Param.oms = 103.2175;
	data.Param.ps = 149877773839.2;
	data.Param.es = 0.01669902;
	data.Param.bigoms = 0;
	data.Param.incs = 23.44;
	data.u1s = 31.88;
	data.Param.omm = 103.2175;
	data.Param.pm = 149877773839.2;
	data.Param.em = 0.01669902;
	data.Param.bigomm = 0;
	data.Param.incm = 23.44;
	data.u1m = 31.88;
	data.Param.hour = 13;
	data.Param.min = 43;
	data.Param.sec = 13;
	data.Param.day = 17;
	data.Param.month = 10;
	data.Param.year = 2017;
	data.Param.Angle_pitch = 0;
	data.Param.Angle_roving = 0;
	data.Param.Angle_bank = 0;
	data.Param.omega_x = 0;
	data.Param.omega_y = 0;
	data.Param.omega_z = 0;
	data.Param.latitude = 0;
	data.a0 = 0;
	data.a1 = 0;
	data.a2 = 0;
	data.a3 = 0;
	data.as = 0;
	data.M0s = 0;
	data.am = 0;
	data.M0m = 0;


	/*****************************************/

	for (main_counter = 0; main_counter < 8; main_counter++)
	{
		printf("		Iteration #%d\n", main_counter);
        
		clock_gettime(CLOCK_REALTIME, &prevclockval);
#ifdef __QNX__
		time (&t1);
		aa = ClockCycles ();
#endif

/*
		for (i = 0; i < NumSamples; i++)
		{
			cycs[i] = ClockCycles ();

		}
*/
			if (main_counter > 0)
				init = 0;

		user_entry_point();

        clock_gettime(CLOCK_REALTIME, &clockval);
#ifdef __QNX__
        bb = ClockCycles ();
        time (&t2);
#endif

		delta = ((uint64_t) clockval.tv_sec * BILLION + (uint64_t) clockval.tv_nsec)
						- ((uint64_t) prevclockval.tv_sec * BILLION + (uint64_t) prevclockval.tv_nsec);
		/*
		 *  now, print out the delta's, so we can easily compute how much
		 *  time each "for" loop iteration took.
		 */
#ifdef __QNX__
		cyclespersec = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
		cyclespermicrosec = (SYSPAGE_ENTRY(qtime)->cycles_per_sec) / MILLION;
		cyclespernanosec = (SYSPAGE_ENTRY(qtime)->cycles_per_sec) / BILLION;
#endif

	//	printf("clock cycles value is %lld\n", cyclespersec);
	//	printf("@for loop@ took %lld microseconds (ClockCycles)\n", (cycs[29] - cycs[0]) / cyclespermicrosec);
	//	printf("@for loop@ took %lld nanoseconds (ClockCycles)\n", (cycs[29] - cycs[0]) / cyclespernanosec );
		printf("@polis_proc@ took %lld microseconds (gettime)\n", delta/1000);
#ifdef __QNX__
		printf("@polis_proc@ took %lld microseconds (ClockCycles above)\n", (bb - aa) / cyclespermicrosec);
	//	printf("@for loop@ took %lld nanoseconds (ClockCycles above)\n", (bb - aa) / cyclespernanosec );
		printf("@polis_proc@ took %.0f seconds (difftime)\n", difftime(t2, t1));
#endif
	}

	printf("%s:  main, exiting\n", progname);
	return EXIT_SUCCESS;
}



/*constants*/
#define PI 				3.1415926535897932384626433832795
#define PI2				6.283185307179586476925286766559

#define EARTH_RADIUS	6378168.00

#define EARTH_MASS 		(5.9726 * pow(10.0, 24.0))

#define J2 				(1082625.7 * pow(10.0, -9.0))

#define G 				(6.67408 * pow(10.0, -11.0))

#define T				86164.090530833

#define wE				(2.0 * PI / T)

#define muE				(EARTH_MASS * G)

#define ANGLE_PITCH		0
#define ANGLE_ROVING	0
#define ANGLE_BANK		0

#define Re				EARTH_RADIUS
#define MU0				(4 * PI * pow(10,(-7)))   // Absolute magnetic constant
//#define r				1

#define L_X				0.03
#define L_Y				0.03
#define L_Z				0.03

#define J_X				150
#define J_Y				1200
#define J_Z				1200
#define J_XZ			1
#define J_XY			(-35)
#define J_YZ			1
#define J_YX			1
#define J_ZY			1
#define J_ZX			1

#define OMEGA0			0
#define OMEGAX			0
#define OMEGAY			0
#define OMEGAZ			0

/*For the Earth*/
#define MUe		3.98602 * pow(10,14)
#define EPS		2.634 * pow(10,25)
#define OmE		7.29211 * pow(10,(-5))
/*For the Sun*/
#define MUs		1.32718 * pow(10,20)
/* For the Moon*/
#define MUm		4.903 * pow(10,12)
/*Math constants*/
#define RtD		(180.0/ PI)
#define DtR		(PI/180.0)
#define date0_tm_sec 	( 20 )
#define date0_tm_min 	( 20 )
#define date0_tm_hour 	( 13 )
#define date0_tm_mday	( 1 )
#define date0_tm_mon	( 3 )
#define date0_tm_year	( 2017 )
#define day_start	( 1 )
#define mon_start	( 1 )
#define year_start	( 2017 )

const int IntOfSunlow[10] = { 65, 75, 100, 125, 150, 175, 200, 225, 250, 275 };
const double Coefflow[10][3] = { { -15.77005, 0.78319, 70.58367 }, { -17.09296,
		0.71004, 90.20671 }, { -17.46898, 0.66923, 91.34157 }, { -17.63874,
		0.63195, 91.67450 }, { -18.70041, 0.57145, 110.48925 }, { -18.99288,
		0.54002, 113.48581 }, { -19.52745, 0.50286, 122.44500 }, { -19.77968,
		0.47717, 126.18335 }, { -20.19715, 0.44589, 133.40432 }, { -20.35393,
		0.42793, 135.74445 } };
const double CoeffHight[10][3] = { { -32.22560, 0.21621, 538.48084 }, {
		-32.42034, 0.20569, 580.71850 }, { -31.01021, 0.23741, 557.19285 }, {
		-28.70956, 0.29505, 481.67084 }, { -25.85660, 0.35861, 368.99154 }, {
		-23.32353, 0.40080, 243.19499 }, { -15.74303, 0.54457, -126.70696 }, {
		-5.36289, 0.67802, -728.20718 }, { 1.83527, 0.76169, -1115.59822 }, {
		16.90856, 0.90769, -2002.33302 } };



/*Parametrs of SAT orbit*/
/*Radius-vector*/
double r(double x, double y, double z) {
	return (sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2)));
}
/*FULL VELOCITY*/
double v(double vx, double vy, double vz) {
	return (sqrt(pow(vx, 2) + pow(vy, 2) + pow(vz, 2)));
}
/*INFLUENCE OF THE EARTH ATMOSPHERE*/
double vv(double x, double y, double z, double vx, double vy, double vz) {
	double a = vx - OmE * y;
	double b = vy - OmE * x;
	double c = vz;
	return (pow(a, 2) + pow(b, 2) + pow(c, 2));
}

/*������� ��������� ���������*/
double ro(double x, double y, double z, double a0, double a1, double a2,
		double a3) {
	double R = r(x, y, z);
	double h = (R - 6378245 * (1 - 0.00335233 * pow(z, 2) / pow(R, 2))) / 1000;
	double x1 = h - a3;
	/*if (x1 < 0) {
	 exit(1);
	 }*/
	double x2 = a2 * pow(x1, 0.5);
	double x3 = a1 - x2;
	double x4 = exp(x3);
	return (a0 * x4);
}
/*������ ������������� ���������*/
double F2_x(double x, double y, double z, double vx, double vy, double vz,
		double a0, double a1, double a2, double a3, double Sm, double Cx,
		double m) {
	double a = -Sm * Cx / (2 * m);
	double Ro = ro(x, y, z, a0, a1, a2, a3);
	double Vv = vv(x, y, z, vx, vy, vz);
	double b = Ro * Vv;
	double c = vx / v(vx, vy, vz);
	return (a * b * c);
}
double F2_y(double x, double y, double z, double vx, double vy, double vz,
		double a0, double a1, double a2, double a3, double Sm, double Cx,
		double m) {
	double a = -Sm * Cx / (2 * m);
	double Ro = ro(x, y, z, a0, a1, a2, a3);
	double Vv = vv(x, y, z, vx, vy, vz);
	double b = Ro * Vv;
	double c = vy / v(vx, vy, vz);
	return (a * b * c);
}

double F2_z(double x, double y, double z, double vx, double vy, double vz,
		double a0, double a1, double a2, double a3, double Sm, double Cx,
		double m) {
	double a = -Sm * Cx / (2 * m);
	double Ro = ro(x, y, z, a0, a1, a2, a3);
	double Vv = vv(x, y, z, vx, vy, vz);
	double b = Ro * Vv;
	double c = vz / v(vx, vy, vz);
	return (a * b * c);
}

double F1_x(double x, double y, double z) {
	double R = r(x, y, z);
	double a = -x / (pow(R, 3));
	double b = EPS / (pow(R, 2));
	double c = ((5 * pow(z, 2)) / (pow(R, 2))) - 1;
	return (a * ( MUe - b * c));
}

double F1_y(double x, double y, double z) {
	double R = r(x, y, z);
	double a = -y / (pow(R, 3));
	double b = EPS / (pow(R, 2));
	double c = ((5 * pow(z, 2)) / (pow(R, 2))) - 1;
	return (a * ( MUe - b * c));
}

double F1_z(double x, double y, double z) {
	double R = r(x, y, z);
	double a = -z / (pow(R, 3));
	double b = EPS / (pow(R, 2));
	double c = ((5 * pow(z, 2)) / (pow(R, 2))) - 3;
	return (a * ( MUe - b * c));
}

/*COORDINATE*/
double dx(double vx) {
	return (vx);
}

double dy(double vy) {
	return (vy);
}

double dz(double vz) {
	return (vz);
}

double interpol(double Fs, double IntOfSun1, double IntOfSun2, double Coeff1,
		double Coef2) {
	return (((Fs - IntOfSun1) * (Coef2 - Coeff1) / (IntOfSun2 - IntOfSun1))
			+ Coeff1);
}

double M(double MU, double a, double M0, double t0, double t) {
	double k1 = MU / (pow(a, 3));
	double k2 = pow(k1, 0.5);
	double k3 = t - t0;
	double k4 = k2 * k3;
	return (k4 + M0);
}
double E(double e, double a, double M0, double t0, double t, double MU) {
	double M_ = M(MU, a, M0, t0, t);
	return (M_ / (1 - e));
}
double tetta(double e, double a, double M0, double t0, double t, double MU) {
	double E_ = E(e, a, M0, t0, t, MU);
	double k1 = (1 + e) / (1 - e);
	double k2 = pow(k1, 0.5);
	double k3 = tan(E_ / 2);
	return (2 * atan(k2 * k3));
}
double u(double e, double a, double M0, double om, double t0, double t,
		double MU) {
	double tetta_ = tetta(e, a, M0, t0, t, MU);
	return (tetta_ + om);
}
double rorb(double p, double e, double a, double M0, double t0, double t,
		double MU) {
	double tetta_ = tetta(e, a, M0, t0, t, MU);
	double k1 = 1 + e * cos(tetta_);
	return (p / k1);
}

double xi(double inc, double bigom, double p, double e, double a, double M0,
		double om, double t0, double t, double MU) {
	double r_ = rorb(p, e, a, M0, t0, t, MU);
	double u_ = u(e, a, M0, om, t0, t, MU);
	double k1 = cos(bigom) * cos(u_);
	double k2 = sin(bigom) * cos(inc) * sin(u_);
	return (r_ * (k1 - k2));
}
double yi(double inc, double bigom, double p, double e, double a, double M0,
		double om, double t0, double t, double MU) {
	double r_ = rorb(p, e, a, M0, t0, t, MU);
	double u_ = u(e, a, M0, om, t0, t, MU);
	double k1 = sin(bigom) * cos(u_);
	double k2 = cos(bigom) * cos(inc) * sin(u_);
	return (r_ * (k1 + k2));
}
double zi(double inc, double p, double e, double a, double M0, double om,
		double t0, double t, double MU) {
	double r_ = rorb(p, e, a, M0, t0, t, MU);
	double u_ = u(e, a, M0, om, t0, t, MU);
	double k1 = sin(inc);
	double k2 = sin(u_);
	return (r_ * k1 * k2);
}
double r0(double inc, double bigom, double p, double e, double a, double M0,
		double om, double t0, double t, double x, double y, double z, double MU) {
	double xi_ = xi(inc, bigom, p, e, a, M0, om, t0, t, MU);
	double yi_ = yi(inc, bigom, p, e, a, M0, om, t0, t, MU);
	double zi_ = zi(inc, p, e, a, M0, om, t0, t, MU);
	double k1 = pow(xi_ - x, 2);
	double k2 = pow(yi_ - y, 2);
	double k3 = pow(zi_ - z, 2);
	return (pow((k1 + k2 + k3), 0.5));
}
double F4_x(double inc, double bigom, double p, double e, double a, double M0,
		double om, double t0, double t, double x, double y, double z, double Sm,
		double m, double MU) {
	double xi_ = xi(inc, bigom, p, e, a, M0, om, t0, t, MU);
	double r_ = rorb(p, e, a, M0, t0, t, MU);
	double r0_ = r0(inc, bigom, p, e, a, M0, om, t0, t, x, y, z, MU);
	double k1 = (xi_ - x) / (pow(r0_, 3));
	double k2 = xi_ / (pow(r_, 3));
	return (MU * (k1 - k2));
}
double F4_y(double inc, double bigom, double p, double e, double a, double M0,
		double om, double t0, double t, double x, double y, double z, double Sm,
		double m, double MU) {
	double yi_ = yi(inc, bigom, p, e, a, M0, om, t0, t, MU);
	double r_ = rorb(p, e, a, M0, t0, t, MU);
	double r0_ = r0(inc, bigom, p, e, a, M0, om, t0, t, x, y, z, MU);
	double k1 = (yi_ - x) / (pow(r0_, 3));
	double k2 = yi_ / (pow(r_, 3));
	return (MU * (k1 - k2));
}
double F4_z(double inc, double bigom, double p, double e, double a, double M0,
		double om, double t0, double t, double x, double y, double z, double Sm,
		double m, double MU) {
	double zi_ = zi(inc, p, e, a, M0, om, t0, t, MU);
	double r_ = rorb(p, e, a, M0, t0, t, MU);
	double r0_ = r0(inc, bigom, p, e, a, M0, om, t0, t, x, y, z, MU);
	double k1 = (zi_ - z) / (pow(r0_, 3));
	double k2 = zi_ / (pow(r_, 3));
	return (MU * (k1 - k2));
}
double F3_x(double inc, double bigom, double p, double e, double a, double M0,
		double om, double t0, double t, double x, double y, double z, double Sm,
		double m, double MU) {
	double xi_ = xi(inc, bigom, p, e, a, M0, om, t0, t, MU);
	double r_ = rorb(p, e, a, M0, t0, t, MU);
	double r0_ = r0(inc, bigom, p, e, a, M0, om, t0, t, x, y, z, MU);
	double k1 = (1 - 7.42 * pow(10, -4) * Sm / m);
	double k2 = (xi_ - x) / (pow(r0_, 3));
	double k3 = xi_ / (pow(r_, 3));
	return (MU * (k1 * k2 - k3));
}
double F3_y(double inc, double bigom, double p, double e, double a, double M0,
		double om, double t0, double t, double x, double y, double z, double Sm,
		double m, double MU) {
	double yi_ = yi(inc, bigom, p, e, a, M0, om, t0, t, MU);
	double r_ = rorb(p, e, a, M0, t0, t, MU);
	double r0_ = r0(inc, bigom, p, e, a, M0, om, t0, t, x, y, z, MU);
	double k1 = (1 - 7.42 * pow(10, -4) * Sm / m);
	double k2 = (yi_ - y) / (pow(r0_, 3));
	double k3 = yi_ / (pow(r_, 3));
	return (MU * (k1 * k2 - k3));
}
double F3_z(double inc, double bigom, double p, double e, double a, double M0,
		double om, double t0, double t, double x, double y, double z, double Sm,
		double m, double MU) {
	double zi_ = zi(inc, p, e, a, M0, om, t0, t, MU);
	double r_ = rorb(p, e, a, M0, t0, t, MU);
	double r0_ = r0(inc, bigom, p, e, a, M0, om, t0, t, x, y, z, MU);
	double k1 = (1 - 7.42 * pow(10, -4) * Sm / m);
	double k2 = (zi_ - z) / (pow(r0_, 3));
	double k3 = zi_ / (pow(r_, 3));
	return (MU * (k1 * k2 - k3));
}
int two_date(int day_1, int mon_1, int year_1, int day_2, int mon_2, int year_2) {
	struct tm date00;
	struct tm date01;
	date00.tm_hour = 0;
	date00.tm_isdst = 0;
	date00.tm_min = 0;
	date00.tm_sec = 0;
	//date00.tm_wday = 0;
	date00.tm_mday = day_1;
	date00.tm_mon = mon_1 - 1;
	date00.tm_year = (year_1 - 1900);
	date01.tm_hour = 0;
	date01.tm_isdst = 0;
	date01.tm_min = 0;
	date01.tm_sec = 0;
	//date01.tm_wday = 0;
	date01.tm_mday = day_2;
	date01.tm_mon = mon_2 - 1;
	date01.tm_year = (year_2 - 1900);
	int day = (int) (mktime(&date01) - mktime(&date00)) / (60 * 60 * 24);
	return (day);
}
int jdn(int day_1, int mon_1, int year_1, int day_2, int mon_2, int year_2) {
	int day = two_date(day_1, mon_1, year_1, day_2, mon_2, year_2);
	int a = floor((14 - mon_2) / 12.0);
	int y = year_2 + 4800 - a;
	int m = mon_2 + 12 * a - 3;
	return (day + floor((153 * m + 2) / 5.0) + 365 * y + floor(y / 4.0)
			- floor(y / 100.0) + floor(y / 400.0) - 32045);
}
double jd(int day_1, int mon_1, int year_1, int day_2, int mon_2, int year_2,
		int hour_2, int min_2, int sec_2) {
	int jdn_ = jdn(day_1, mon_1, year_1, day_2, mon_2, year_2);
	return (jdn_ + (hour_2 - 12) / 24.0 + min_2 / 1440.0 + sec_2 / 86400.0);
}
double t_st(int day_1, int mon_1, int year_1, int day_2, int mon_2, int year_2,
		int hour_2, int min_2, int sec_2) {
	int jd_ = jd(day_1, mon_1, year_1, day_2, mon_2, year_2, hour_2, min_2,
			sec_2);
	return ((jd_ - 2451545.0) / 36525.0);
}
double t0_f_(int day_1, int mon_1, int year_1, int day_2, int mon_2, int year_2,
		int hour_2, int min_2, int sec_2) {
	double t_st_ = t_st(day_1, mon_1, year_1, day_2, mon_2, year_2, hour_2,
			min_2, sec_2);
	return (6.697374558 + 2400.051336 * t_st_ + 0.0000025862 * pow(t_st_, 2));
}
double gst_f(int day_1, int mon_1, int year_1, int day_2, int mon_2, int year_2,
		int hour_2, int min_2, int sec_2) {
	double t0_f__ = t0_f_(day_1, mon_1, year_1, day_2, mon_2, year_2, hour_2,
			min_2, sec_2);
	return (t0_f__ + (hour_2 + (min_2 / 60) + (sec_2 / 3600)) * 1.002737909);
}
// Calculation of elements of a matrix of the directing cosines
double coef_a11(double f_Angle_pitch, double f_Angle_roving) {
	return (cos(f_Angle_pitch) * cos(f_Angle_roving));
}
double coef_a12(double f_Angle_pitch, double f_Angle_roving) {
	return (sin(f_Angle_pitch) * cos(f_Angle_roving));
}
double coef_a13(double f_Angle_roving) {
	return (-sin(f_Angle_roving));
}
double coef_a21(double f_Angle_pitch, double f_Angle_roving,
		double f_Angle_bank) {
	return (-cos(f_Angle_bank) * sin(f_Angle_pitch)
			+ sin(f_Angle_bank) * sin(f_Angle_roving) * cos(f_Angle_pitch));
}
double coef_a22(double f_Angle_pitch, double f_Angle_roving,
		double f_Angle_bank) {
	return (cos(f_Angle_bank) * cos(f_Angle_pitch)
			+ sin(f_Angle_bank) * sin(f_Angle_roving) * sin(f_Angle_pitch));
}
double coef_a23(double f_Angle_roving, double f_Angle_bank) {
	return (sin(f_Angle_bank) * cos(f_Angle_roving));
}
double coef_a31(double f_Angle_pitch, double f_Angle_roving,
		double f_Angle_bank) {
	return (cos(f_Angle_bank) * sin(f_Angle_roving) * cos(f_Angle_pitch)
			+ sin(f_Angle_bank) * sin(f_Angle_pitch));
}
double coef_a32(double f_Angle_pitch, double f_Angle_roving,
		double f_Angle_bank) {
	return (cos(f_Angle_bank) * sin(f_Angle_roving) * sin(f_Angle_pitch)
			- sin(f_Angle_bank) * cos(f_Angle_pitch));
}
double coef_a33(double f_Angle_roving, double f_Angle_bank) {
	return (cos(f_Angle_bank) * cos(f_Angle_roving));
}
double b_0(double x, double y, double z) {
	return ((MU0 * EARTH_MASS)
			/ (4 * PI
					* sqrt(
							(x * x + y * y + z * z) * (x * x + y * y + z * z)
									* (x * x + y * y + z * z))));
}
// Calculation of projections of a vector of magnetic induction
double b_x0(double f_inc, double f_latitude, double x, double y, double z) {
	double B0 = b_0(x, y, z);
	return (-2 * B0 * sin(f_inc) * sin(f_latitude));
}
double b_y0(double f_inc, double f_latitude, double x, double y, double z) {
	double B0 = b_0(x, y, z);
	return (B0 * sin(f_inc) * cos(f_latitude));
}
double b_z0(double f_inc, double f_latitude, double x, double y, double z) {
	double B0 = b_0(x, y, z);
	return (B0 * cos(f_inc));
}
//Calculation of projections of a vector of magnetic induction (related systems of coordinates)
double b_x(double f_Angle_pitch, double f_Angle_roving, double f_inc,
		double f_latitude, double x, double y, double z) {
	double f_a11 = coef_a11(f_Angle_pitch, f_Angle_roving);
	double f_a12 = coef_a12(f_Angle_pitch, f_Angle_roving);
	double f_a13 = coef_a13(f_Angle_roving);
	double f_b_x0 = b_x0(f_inc, f_latitude, x, y, z);
	double f_b_y0 = b_y0(f_inc, f_latitude, x, y, z);
	double f_b_z0 = b_z0(f_inc, f_latitude, x, y, z);
	return (f_a11 * f_b_x0 + f_a12 * f_b_y0 + f_a13 * f_b_z0);
}
double b_y(double f_Angle_pitch, double f_Angle_roving, double f_Angle_bank,
		double f_inc, double f_latitude, double x, double y, double z) {
	double f_a21 = coef_a21(f_Angle_pitch, f_Angle_roving, f_Angle_bank);
	double f_a22 = coef_a22(f_Angle_pitch, f_Angle_roving, f_Angle_bank);
	double f_a23 = coef_a23(f_Angle_roving, f_Angle_bank);
	double f_b_x0 = b_x0(f_inc, f_latitude, x, y, z);
	double f_b_y0 = b_y0(f_inc, f_latitude, x, y, z);
	double f_b_z0 = b_z0(f_inc, f_latitude, x, y, z);
	return (f_a21 * f_b_x0 + f_a22 * f_b_y0 + f_a23 * f_b_z0);
}
double b_z(double f_Angle_pitch, double f_Angle_roving, double f_Angle_bank,
		double f_inc, double f_latitude, double x, double y, double z) {
	double f_a31 = coef_a31(f_Angle_pitch, f_Angle_roving, f_Angle_bank);
	double f_a32 = coef_a32(f_Angle_pitch, f_Angle_roving, f_Angle_bank);
	double f_a33 = coef_a33(f_Angle_roving, f_Angle_bank);
	double f_b_x0 = b_x0(f_inc, f_latitude, x, y, z);
	double f_b_y0 = b_y0(f_inc, f_latitude, x, y, z);
	double f_b_z0 = b_z0(f_inc, f_latitude, x, y, z);
	return (f_a31 * f_b_x0 + f_a32 * f_b_y0 + f_a33 * f_b_z0);
}
//Calculation of projections of the magnetic revolting moment
double m_magn_x(double f_Angle_pitch, double f_Angle_roving,
		double f_Angle_bank, double f_inc, double f_latitude, double x,
		double y, double z) {
	double f_b_z = b_z(f_Angle_pitch, f_Angle_roving, f_Angle_bank, f_inc,
			f_latitude, x, y, z);
	double f_b_y = b_y(f_Angle_pitch, f_Angle_roving, f_Angle_bank, f_inc,
			f_latitude, x, y, z);
	return (L_Y * f_b_z - L_Z * f_b_y);
}
double m_magn_y(double f_Angle_pitch, double f_Angle_roving,
		double f_Angle_bank, double f_inc, double f_latitude, double x,
		double y, double z) {
	double f_b_z = b_z(f_Angle_pitch, f_Angle_roving, f_Angle_bank, f_inc,
			f_latitude, x, y, z);
	double f_b_x = b_x(f_Angle_pitch, f_Angle_roving, f_inc, f_latitude, x, y,
			z);
	return (L_Z * f_b_x - L_X * f_b_z);
}
double m_magn_z(double f_Angle_pitch, double f_Angle_roving,
		double f_Angle_bank, double f_inc, double f_latitude, double x,
		double y, double z) {
	double f_b_z = b_z(f_Angle_pitch, f_Angle_roving, f_Angle_bank, f_inc,
			f_latitude, x, y, z);
	double f_b_y = b_y(f_Angle_pitch, f_Angle_roving, f_Angle_bank, f_inc,
			f_latitude, x, y, z);
	return (L_X * f_b_y - L_Y * f_b_z);
}
//Grav moment
double m_x(double f_Angle_pitch, double f_Angle_roving, double f_Angle_bank) {
	double f_a22 = coef_a22(f_Angle_pitch, f_Angle_roving, f_Angle_bank);
	double f_a32 = coef_a32(f_Angle_pitch, f_Angle_roving, f_Angle_bank);
	double f_a12 = coef_a12(f_Angle_pitch, f_Angle_roving);
	return ((-3) * OMEGA0 * OMEGA0
			* ((J_Y - J_Z) * f_a22 * f_a32
					+ (J_XZ * f_a22 - J_XY * f_a32) * f_a12
					+ J_YZ * (f_a22 * f_a22 - f_a32 * f_a32)));
}
double m_y(double f_Angle_pitch, double f_Angle_roving, double f_Angle_bank) {
	double f_a22 = coef_a22(f_Angle_pitch, f_Angle_roving, f_Angle_bank);
	double f_a32 = coef_a32(f_Angle_pitch, f_Angle_roving, f_Angle_bank);
	double f_a12 = coef_a12(f_Angle_pitch, f_Angle_roving);
	return ((-3) * OMEGA0 * OMEGA0
			* ((J_Z - J_X) * f_a32 * f_a12
					+ (J_YX * f_a32 - J_YZ * f_a12) * f_a22
					+ J_XZ * (f_a32 * f_a32 - f_a12 * f_a12)));
}
double m_z(double f_Angle_pitch, double f_Angle_roving, double f_Angle_bank) {
	double f_a22 = coef_a22(f_Angle_pitch, f_Angle_roving, f_Angle_bank);
	double f_a32 = coef_a32(f_Angle_pitch, f_Angle_roving, f_Angle_bank);
	double f_a12 = coef_a12(f_Angle_pitch, f_Angle_roving);
	return ((-3) * OMEGA0 * OMEGA0
			* ((J_X - J_Y) * f_a12 * f_a22
					+ (J_ZY * f_a12 - J_ZX * f_a22) * f_a32
					+ J_YZ * (f_a12 * f_a12 - f_a22 * f_a22)));
}
//Calculating of the revolting moment
double a_x_(double f_omega_x, double f_omega_y, double f_omega_z) {
	double summ = 0;
	double om1 = f_omega_x * f_omega_y;
	double om2 = f_omega_z * f_omega_z;
	double om3 = f_omega_y * f_omega_y;
	summ += om1 * (J_Z - J_Y);
	summ += J_YZ * (om2 - om3);
	summ += f_omega_x * (J_XY * f_omega_z - J_XZ * f_omega_y);
	return (summ);
}
double a_y_(double f_omega_x, double f_omega_y, double f_omega_z) {
	double summ = 0;
	double om1 = -f_omega_z * f_omega_x;
	double om2 = f_omega_z * f_omega_z;
	double om3 = f_omega_x * f_omega_x;
	summ += om1 * (J_Z - J_X);
	summ += J_XZ * (om2 - om3);
	summ += f_omega_y * (J_XY * f_omega_z - J_YZ * f_omega_x);
	return (summ);
}
double a_z_(double f_omega_x, double f_omega_y, double f_omega_z) {
	double summ = 0;
	double om1 = f_omega_y * f_omega_x;
	double om2 = f_omega_y * f_omega_y;
	double om3 = f_omega_x * f_omega_x;
	summ += om1 * (J_Y - J_X);
	summ += J_XY * (om2 - om3);
	summ += f_omega_y * (J_XY * f_omega_y - J_YZ * f_omega_x);
	return (summ);
}
double c_x(double f_Angle_pitch, double f_Angle_roving, double f_Angle_bank,
		double f_inc, double f_latitude, double x, double y, double z,
		double f_omega_x, double f_omega_y, double f_omega_z) {
	double f_magn_x = m_magn_x(f_Angle_pitch, f_Angle_roving, f_Angle_bank,
			f_inc, f_latitude, x, y, z);
	double f_m_x = m_x(f_Angle_pitch, f_Angle_roving, f_Angle_bank);
	return (f_magn_x + f_m_x - a_x_(f_omega_x, f_omega_y, f_omega_z));
}
double c_y(double f_Angle_pitch, double f_Angle_roving, double f_Angle_bank,
		double f_inc, double f_latitude, double x, double y, double z,
		double f_omega_x, double f_omega_y, double f_omega_z) {
	double f_magn_y = m_magn_y(f_Angle_pitch, f_Angle_roving, f_Angle_bank,
			f_inc, f_latitude, x, y, z);
	double f_m_y = m_y(f_Angle_pitch, f_Angle_roving, f_Angle_bank);
	return (f_magn_y + f_m_y - a_y_(f_omega_x, f_omega_y, f_omega_z));
}
double c_z(double f_Angle_pitch, double f_Angle_roving, double f_Angle_bank,
		double f_inc, double f_latitude, double x, double y, double z,
		double f_omega_x, double f_omega_y, double f_omega_z) {
	double f_magn_z = m_magn_z(f_Angle_pitch, f_Angle_roving, f_Angle_bank,
			f_inc, f_latitude, x, y, z);
	double f_m_z = m_z(f_Angle_pitch, f_Angle_roving, f_Angle_bank);
	return (f_magn_z + f_m_z - a_z_(f_omega_x, f_omega_y, f_omega_z));
}
double d_omega_x(double f_Angle_pitch, double f_Angle_roving,
		double f_Angle_bank, double f_inc, double f_latitude, double x,
		double y, double z, double f_omega_x, double f_omega_y,
		double f_omega_z) {
	double f_j = (J_X * J_Y * J_Z - J_X * J_YZ * J_YZ - J_XY * J_XY * J_X
			- J_XY * J_XY * J_XZ - J_XZ * J_XY * J_YZ - J_XZ * J_Y * J_XZ);
	double f_cx = c_x(f_Angle_pitch, f_Angle_roving, f_Angle_bank, f_inc,
			f_latitude, x, y, z, f_omega_x, f_omega_y, f_omega_z);
	double f_cy = c_y(f_Angle_pitch, f_Angle_roving, f_Angle_bank, f_inc,
			f_latitude, x, y, z, f_omega_x, f_omega_y, f_omega_z);
	double f_cz = c_z(f_Angle_pitch, f_Angle_roving, f_Angle_bank, f_inc,
			f_latitude, x, y, z, f_omega_x, f_omega_y, f_omega_z);
	return (((-1) * f_cx * (J_Y * J_Z - J_YZ * J_YZ)
			+ f_cy * (J_Z * J_XY + J_XZ * J_YZ)
			+ f_cz * (J_Y * J_XZ + J_XY * J_YZ)) / f_j);
}
double d_omega_y(double f_Angle_pitch, double f_Angle_roving,
		double f_Angle_bank, double f_inc, double f_latitude, double x,
		double y, double z, double f_omega_x, double f_omega_y,
		double f_omega_z) {
	double f_j = (J_X * J_Y * J_Z - J_X * J_YZ * J_YZ - J_XY * J_XY * J_X
			- J_XY * J_XY * J_XZ - J_XZ * J_XY * J_YZ - J_XZ * J_Y * J_XZ);
	double f_cx = c_x(f_Angle_pitch, f_Angle_roving, f_Angle_bank, f_inc,
			f_latitude, x, y, z, f_omega_x, f_omega_y, f_omega_z);
	double f_cy = c_y(f_Angle_pitch, f_Angle_roving, f_Angle_bank, f_inc,
			f_latitude, x, y, z, f_omega_x, f_omega_y, f_omega_z);
	double f_cz = c_z(f_Angle_pitch, f_Angle_roving, f_Angle_bank, f_inc,
			f_latitude, x, y, z, f_omega_x, f_omega_y, f_omega_z);
	return ((-1)
			* ((-1) * f_cy * (J_X * J_Z - J_XZ * J_XZ)
					+ f_cx * (J_Z * J_XY + J_XZ * J_YZ)
					+ f_cz * (J_X * J_YZ + J_XY * J_XZ)) / f_j);
}
double d_omega_z(double f_Angle_pitch, double f_Angle_roving,
		double f_Angle_bank, double f_inc, double f_latitude, double x,
		double y, double z, double f_omega_x, double f_omega_y,
		double f_omega_z) {
	double f_j = (J_X * J_Y * J_Z - J_X * J_YZ * J_YZ - J_XY * J_XY * J_X
			- J_XY * J_XY * J_XZ - J_XZ * J_XY * J_YZ - J_XZ * J_Y * J_XZ);
	double f_cx = c_x(f_Angle_pitch, f_Angle_roving, f_Angle_bank, f_inc,
			f_latitude, x, y, z, f_omega_x, f_omega_y, f_omega_z);
	double f_cz = c_z(f_Angle_pitch, f_Angle_roving, f_Angle_bank, f_inc,
			f_latitude, x, y, z, f_omega_x, f_omega_y, f_omega_z);
	return (((-1) * f_cz * (J_X * J_Y - J_XY * J_XY)
			+ f_cx * (J_Y * J_XZ + J_XY * J_YZ)
			+ f_cz * (J_X * J_YZ + J_XY * J_XZ)) / f_j);
}
double omega_1_x(double f_Angle_roving, double f_omega_x) {
	double f_a13 = coef_a13(f_Angle_roving);
	return (f_omega_x - f_a13 * OMEGA0);
}
double omega_1_y(double f_Angle_roving, double f_Angle_bank, double f_omega_y) {
	double f_a23 = coef_a23(f_Angle_roving, f_Angle_bank);
	return (f_omega_y - f_a23 * OMEGA0);
}
double omega_1_z(double f_Angle_roving, double f_Angle_bank, double f_omega_z) {
	double f_a33 = coef_a33(f_Angle_roving, f_Angle_bank);
	return (f_omega_z - f_a33 * OMEGA0);
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
double d_angle_bank(double f_Angle_roving, double f_Angle_bank,
		double f_omega_x, double f_omega_y, double f_omega_z) {
	double f_omega_1_x = omega_1_x(f_Angle_roving, f_omega_x);
	double f_omega_1_y = omega_1_y(f_Angle_roving, f_Angle_bank, f_omega_y);
	double f_omega_1_z = omega_1_z(f_Angle_roving, f_Angle_bank, f_omega_z);
	return (f_omega_1_x
			+ tan(ANGLE_ROVING)
					* (f_omega_1_z * cos(ANGLE_BANK)
							+ f_omega_1_y * sin(ANGLE_BANK)));
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
double d_angle_roving(double f_Angle_roving, double f_Angle_bank,
		double f_omega_x, double f_omega_y, double f_omega_z) {
	double f_omega_1_y = omega_1_y(f_Angle_roving, f_Angle_bank, f_omega_y);
	double f_omega_1_z = omega_1_z(f_Angle_roving, f_Angle_bank, f_omega_z);
	return (f_omega_1_y * cos(ANGLE_BANK) - f_omega_1_z * sin(ANGLE_BANK));
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
double d_angle_pitch(double f_Angle_roving, double f_Angle_bank,
		double f_omega_x, double f_omega_y, double f_omega_z) {
	double f_omega_1_y = omega_1_y(f_Angle_roving, f_Angle_bank, f_omega_y);
	double f_omega_1_z = omega_1_z(f_Angle_roving, f_Angle_bank, f_omega_z);
	return ((f_omega_1_z * cos(ANGLE_BANK) - f_omega_1_y * sin(ANGLE_BANK))
			/ cos(ANGLE_ROVING));
}
double dvx(double x, double y, double z, double vx, double vy, double vz,
		double a0, double a1, double a2, double a3, double Sm, double Cx,
		double m, double incs, double bigoms, double ps, double es, double as,
		double M0s, double oms, double t, double t0, double incm, double bigomm,
		double pm, double em, double am, double M0m, double omm) {
	double a = F1_x(x, y, z);
	double b = F2_x(x, y, z, vx, vy, vz, a0, a1, a2, a3, Sm, Cx, m);
	double c = F3_x(incs, bigoms, ps, es, as, M0s, oms, t0, t, x, y, z, Sm, m,
			MUs);
	double d = F4_x(incm, bigomm, pm, em, am, M0m, omm, t0, t, x, y, z, Sm, m,
			MUm);
	return (a + b + c + d);
}
double dvy(double x, double y, double z, double vx, double vy, double vz,
		double a0, double a1, double a2, double a3, double Sm, double Cx,
		double m, double incs, double bigoms, double ps, double es, double as,
		double M0s, double oms, double t, double t0, double incm, double bigomm,
		double pm, double em, double am, double M0m, double omm) {
	double a = F1_y(x, y, z);
	double b = F2_y(x, y, z, vx, vy, vz, a0, a1, a2, a3, Sm, Cx, m);
	double c = F3_y(incs, bigoms, ps, es, as, M0s, oms, t0, t, x, y, z, Sm, m,
			MUs);
	double d = F4_y(incm, bigomm, pm, em, am, M0m, omm, t0, t, x, y, z, Sm, m,
			MUm);
	return (a + b + c + d);
}
double dvz(double x, double y, double z, double vx, double vy, double vz,
		double a0, double a1, double a2, double a3, double Sm, double Cx,
		double m, double incs, double bigoms, double ps, double es, double as,
		double M0s, double oms, double t, double t0, double incm, double bigomm,
		double pm, double em, double am, double M0m, double omm) {
	double a = F1_z(x, y, z);
	double b = F2_z(x, y, z, vx, vy, vz, a0, a1, a2, a3, Sm, Cx, m);
	double c = F3_z(incs, bigoms, ps, es, as, M0s, oms, t0, t, x, y, z, Sm, m,
			MUs);
	double d = F4_z(incm, bigomm, pm, em, am, M0m, omm, t0, t, x, y, z, Sm, m,
			MUm);
	return (a + b + c + d);
}

//-----------------------------------------------------------------------------


clock_t start;
clock_t finish;

double time_pr;

///Функция с пользовательским кодом
///возвращает код ошибки пользователя
///аргументами принимает:
///первыми по очереди идут данные с портов приема
///за ними - данные на порт отправки
///причем, порты приема обозначаются,
///как приставка "data_in_"+порядковый номер порта,
///порты на отправку обозначаются,
///как приставка "data_out_"+порядковый номер порта,
int user_entry_point(void)
{ ///Код пользователя/////////////////////////////////////////////////////НАЧАЛО

	//пользовательские данные для работы внутри функции user_entry_point
//	struct OrbParam data;
	//данные пришедшие из вне оформленные в пользовательском виде
	struct OrbParam *data_in = &data;
	/*
	printf("\ndata_in.init = %d\n", data_in->init);
	printf("\ndata_in.Param.x = %f\n", data_in->Param.x);
	 */

	struct tm date0;
	time_t sec_start;
	/*
	start = clock();
*/


	double a0;
	double a1;
	double a2;
	double a3;
	double fies;
	double lyames;
	double hes;
	double x; /*���������� � �����*/
	double y;
	double z;
	double R; /*������ - ������ ��*/
	double inc; /*���������� ������*/
	double hs; /*�������������� ������ ������*/
	int itog;
	double graditorad;
	double radsumm;
	double vx; /*�������� ���������� ������� �������� �� �����*/
	double vy;
	double vz;
	double V;
	double latitude;
	double Angle_pitch;
	double Angle_bank;
	double Angle_roving;
	double omega_x;
	double omega_y;
	double omega_z;
	/*����� � ���*/
	double t0;
	double t;
	double dt;

	/*�������-���������� �������������� ��*/
	double Sm;
	double Cx;
	double m;
	int i;
	int j;

	/*������������ ��� ���������*/
	double Fs; 	//��������� ����������

	double CoeffofAtm[3];

	/*��������� ��������� ��� ������� ������ ������*/
	double oms;
	double ps;				//�������� ������
	double es;				//��������������
	double bigoms;			//������� ����������� ����
	double incs;			//����������
	double u1s;				//�������� �������

	double tetta1s;

	double k1;
	double k2;
	double k3;
	double E0s;

	double M0s;

	double as;

	/*��������� ��������� ��� ������� ������ ����*/
	double omm;
	double pm;
	double em;
	double bigomm;
	double incm;
	double u1m;

	double tetta1m;

	double k1m;
	double k2m;
	double k3m;
	double E0m;

	double M0m;

	double am;

	if (init == 1) {

		x = data_in->Param.x; /*���������� � �����*/
		y = data_in->Param.y;
		z = data_in->Param.z;
		R = r(x, y, z); /*������ - ������ ��*/
		inc = data_in->Param.inc; /*���������� ������*/
		hs = (R - Re) / 1000; /*�������������� ������ ������*/
		graditorad = inc * (DtR);
		radsumm = graditorad;
		vx = data_in->Param.vx; /*�������� ���������� ������� �������� �� �����*/
		vy = data_in->Param.vy;
		vz = data_in->Param.vz;
		V = v(vx, vy, vz);

		/*����� � ���*/
		t0 = data_in->Param.t0;
		t = data_in->Param.t;
		dt = data_in->Param.dt;

		/*�������-���������� �������������� ��*/
		Sm = data_in->Param.Sm;
		Cx = data_in->Param.Cx;
		m = data_in->Param.m;

		/*������������ ��� ���������*/
		Fs = data_in->Param.Fs; 					//��������� ����������

		if (hs <= 600) {
			for (i = 0; i < 10; i++) {
				if ((Fs > IntOfSunlow[i]) && (Fs < IntOfSunlow[i + 1])) {
					for (j = 0; j < 3; j++) {
						CoeffofAtm[j] = interpol(Fs, IntOfSunlow[i],
								IntOfSunlow[i + 1], Coefflow[i][j],
								Coefflow[i + 1][j]);
					}
				}
				a0 = 9.80665;
				a1 = CoeffofAtm[0];
				a2 = CoeffofAtm[1];
				a3 = CoeffofAtm[2];
			}
		} else if ((hs > 600) && (hs < 1500)) {
			for (i = 0; i < 10; i++) {
				if ((Fs > IntOfSunlow[i]) && (Fs < IntOfSunlow[i + 1])) {
					for (j = 0; j < 3; j++) {
						CoeffofAtm[j] = interpol(Fs, IntOfSunlow[i],
								IntOfSunlow[i + 1], CoeffHight[i][j],
								CoeffHight[i + 1][j]);
					}
				}
				a0 = 9.80665;
				a1 = CoeffofAtm[0];
				a2 = CoeffofAtm[1];
				a3 = CoeffofAtm[2];
			}
		} else {
			a0 = 0;
			a1 = 0;
			a2 = 0;
			a3 = 0;
		}
		data.a0 = a0;
		data.a1 = a1;
		data.a2 = a2;
		data.a3 = a3;

		/*��������� ��������� ��� ������� ������ ������*/
		oms = data_in->Param.oms * DtR;
		ps = data_in->Param.ps;				//�������� ������
		es = data_in->Param.es;				//��������������
		bigoms = data_in->Param.bigoms * DtR;		//������� ����������� ����
		incs = data_in->Param.incs * DtR;			//����������
		u1s = data_in->u1s * DtR;			//�������� �������

		tetta1s = u1s - oms;
		k1 = tan(tetta1s / 2);
		k2 = (1 - es) / (1 + es);
		k3 = pow(k2, 0.5);
		E0s = (2 * atan(k1 * k3));
		M0s = E0s - es * E0s;
		as = ps / (1 - pow(es, 2));
		data.as = as;
		data.M0s = M0s;

		/*��������� ��������� ��� ������� ������ ����*/
		omm = data_in->Param.omm * DtR;
		pm = data_in->Param.pm;
		em = data_in->Param.em;
		bigomm = data_in->Param.bigomm * DtR;
		incm = data_in->Param.incm * DtR;
		u1m = data_in->u1m * DtR;
		tetta1m = u1m - omm;
		k1m = tan(tetta1m / 2);
		k2m = (1 - em) / (1 + em);
		k3m = pow(k2m, 0.5);
		E0m = (2 * atan(k1m * k3m));
		M0m = E0m - em * E0m;
		am = pm / (1 - pow(em, 2));
		data.am = am;
		data.M0m = M0m;
		latitude = atan(z / (sqrt(x * x + y * y)));

		data.Param.x = x;
		data.Param.y = y;
		data.Param.z = z;
		data.Param.vx = vx;
		data.Param.vy = vy;
		data.Param.vz = vz;
		data.Param.t = t;
		data.Param.t0 = data_in->Param.t0;
		data.Param.dt = data_in->Param.dt;
		data.Param.Sm = data_in->Param.Sm;
		data.Param.Cx = data_in->Param.Cx;
		data.Param.m = data_in->Param.m;
		data.Param.Fs = data_in->Param.Fs;
		data.Param.oms = oms;
		data.Param.ps = data_in->Param.ps;
		data.Param.es = data_in->Param.es;
		data.Param.bigoms = bigoms;
		data.Param.incs = incs;
		data.Param.omm = omm;
		data.Param.pm = data_in->Param.pm;
		data.Param.em = data_in->Param.em;
		data.Param.bigomm = bigomm;
		data.Param.pm = data_in->Param.pm;
		data.Param.incm = incm;
		data.Param.sec = data_in->Param.sec;
		data.Param.min = data_in->Param.min;
		data.Param.hour = data_in->Param.hour;
		data.Param.day = data_in->Param.day;
		data.Param.month = data_in->Param.month;
		data.Param.year = data_in->Param.year;
		data.Param.latitude = latitude;
		data.Param.Angle_pitch = data_in->Param.Angle_pitch;
		data.Param.Angle_roving = data_in->Param.Angle_roving;
		data.Param.Angle_bank = data_in->Param.Angle_bank;
		data.Param.omega_x = data_in->Param.omega_x;
		data.Param.omega_y = data_in->Param.omega_y;
		data.Param.omega_z = data_in->Param.omega_z;

//		printf("init - ok\n");
	} else {
		x = data_in->Param.x;
		y = data_in->Param.y;
		z = data_in->Param.z;
		vx = data_in->Param.vx;
		vy = data_in->Param.vy;
		vz = data_in->Param.vz;
		a0 = data_in->a0;
		a1 = data_in->a1;
		a2 = data_in->a2;
		a3 = data_in->a3;

		latitude = data_in->Param.latitude;
		Angle_pitch = data_in->Param.Angle_pitch;
		Angle_bank = data_in->Param.Angle_bank;
		Angle_roving = data_in->Param.Angle_roving;

		omega_x = data_in->Param.omega_x;
		omega_y = data_in->Param.omega_y;
		omega_z = data_in->Param.omega_z;

		Sm = data_in->Param.Sm;
		Cx = data_in->Param.Cx;
		m = data_in->Param.m;

		incs = data_in->Param.incs;
		bigoms = data_in->Param.bigoms;
		ps = data_in->Param.ps;
		es = data_in->Param.es;
		as = data_in->as;
		M0s = data_in->M0s;
		oms = data_in->Param.oms;

		/*�������� ������ ��� ������*/
		date0.tm_sec = data_in->Param.sec;
		date0.tm_min = data_in->Param.min;
		date0.tm_hour = data_in->Param.hour;
		date0.tm_mday = data_in->Param.day;
		date0.tm_mon = data_in->Param.month - 1;
		date0.tm_year = (data_in->Param.year - 1900);
		date0.tm_isdst = -1;

		sec_start = mktime(&date0);

		int day_of_pr = date0_tm_mday;							///////UNUSED
		int mon_of_pr = date0_tm_mon;//Date[0];																///////UNUSED
		int year_of_pr = date0_tm_year; //Date[0];															///////UNUSED
		int hour_of_pr = date0_tm_hour;							///////UNUSED
		int min_of_pr = date0_tm_min;							///////UNUSED
		int sec_of_pr = date0_tm_sec;							///////UNUSED

		t = data_in->Param.t;
		t0 = data_in->Param.t0;

		incm = data_in->Param.incm;
		bigomm = data_in->Param.bigomm;
		pm = data_in->Param.pm;
		em = data_in->Param.em;
		am = data_in->am;
		M0m = data_in->M0m;
		omm = data_in->Param.omm;

		dt = data_in->Param.dt;

		double KVx1 = dvx(x, y, z, vx, vy, vz, a0, a1, a2, a3, Sm, Cx, m, incs,
				bigoms, ps, es, as, M0s, oms, t, t0, incm, bigomm, pm, em, am,
				M0m, omm) * dt;
		double KVy1 = dvy(x, y, z, vx, vy, vz, a0, a1, a2, a3, Sm, Cx, m, incs,
				bigoms, ps, es, as, M0s, oms, t, t0, incm, bigomm, pm, em, am,
				M0m, omm) * dt;
		double KVz1 = dvz(x, y, z, vx, vy, vz, a0, a1, a2, a3, Sm, Cx, m, incs,
				bigoms, ps, es, as, M0s, oms, t, t0, incm, bigomm, pm, em, am,
				M0m, omm) * dt;
		double Kx1 = dx(vx) * dt;
		double Ky1 = dy(vy) * dt;
		double Kz1 = dz(vz) * dt;

		double KVx2 = dvx(x + (Kx1) / 2, y + (Ky1) / 2, z + (Kz1) / 2,
				vx + (KVx1) / 2, vy + (KVy1) / 2, vz + (KVz1) / 2, a0, a1, a2,
				a3, Sm, Cx, m, incs, bigoms, ps, es, as, M0s, oms, t, t0, incm,
				bigomm, pm, em, am, M0m, omm) * dt;
		double KVy2 = dvy(x + (Kx1) / 2, y + (Ky1) / 2, z + (Kz1) / 2,
				vx + (KVx1) / 2, vy + (KVy1) / 2, vz + (KVz1) / 2, a0, a1, a2,
				a3, Sm, Cx, m, incs, bigoms, ps, es, as, M0s, oms, t, t0, incm,
				bigomm, pm, em, am, M0m, omm) * dt;
		double KVz2 = dvz(x + (Kx1) / 2, y + (Ky1) / 2, z + (Kz1) / 2,
				vx + (KVx1) / 2, vy + (KVy1) / 2, vz + (KVz1) / 2, a0, a1, a2,
				a3, Sm, Cx, m, incs, bigoms, ps, es, as, M0s, oms, t, t0, incm,
				bigomm, pm, em, am, M0m, omm) * dt;
		double Kx2 = dx(vx + (KVx1) / 2) * dt;
		double Ky2 = dy(vy + (KVy1) / 2) * dt;
		double Kz2 = dz(vz + (KVz1) / 2) * dt;

		double KVx3 = dvx(x + (Kx2) / 2, y + (Ky2) / 2, z + (Kz2) / 2,
				vx + (KVx2) / 2, vy + (KVy2) / 2, vz + (KVz2) / 2, a0, a1, a2,
				a3, Sm, Cx, m, incs, bigoms, ps, es, as, M0s, oms, t, t0, incm,
				bigomm, pm, em, am, M0m, omm) * dt;
		double KVy3 = dvy(x + (Kx2) / 2, y + (Ky2) / 2, z + (Kz2) / 2,
				vx + (KVx2) / 2, vy + (KVy2) / 2, vz + (KVz2) / 2, a0, a1, a2,
				a3, Sm, Cx, m, incs, bigoms, ps, es, as, M0s, oms, t, t0, incm,
				bigomm, pm, em, am, M0m, omm) * dt;
		double KVz3 = dvz(x + (Kx2) / 2, y + (Ky2) / 2, z + (Kz2) / 2,
				vx + (KVx2) / 2, vy + (KVy2) / 2, vz + (KVz2) / 2, a0, a1, a2,
				a3, Sm, Cx, m, incs, bigoms, ps, es, as, M0s, oms, t, t0, incm,
				bigomm, pm, em, am, M0m, omm) * dt;
		double Kx3 = dx(vx + (KVx2) / 2) * dt;
		double Ky3 = dy(vy + (KVy2) / 2) * dt;
		double Kz3 = dz(vz + (KVz2) / 2) * dt;

		double KVx4 = dvx(x + Kx3, y + Ky3, z + Kz3, vx + KVx3, vy + KVy3,
				vz + KVz3, a0, a1, a2, a3, Sm, Cx, m, incs, bigoms, ps, es, as,
				M0s, oms, t, t0, incm, bigomm, pm, em, am, M0m, omm) * dt;
		double KVy4 = dvy(x + Kx3, y + Ky3, z + Kz3, vx + KVx3, vy + KVy3,
				vz + KVz3, a0, a1, a2, a3, Sm, Cx, m, incs, bigoms, ps, es, as,
				M0s, oms, t, t0, incm, bigomm, pm, em, am, M0m, omm) * dt;
		double KVz4 = dvz(x + Kx3, y + Ky3, z + Kz3, vx + KVx3, vy + KVy3,
				vz + KVz3, a0, a1, a2, a3, Sm, Cx, m, incs, bigoms, ps, es, as,
				M0s, oms, t, t0, incm, bigomm, pm, em, am, M0m, omm) * dt;
		double Kx4 = dx(vx + KVx3) * dt;
		double Ky4 = dy(vy + KVy3) * dt;
		double Kz4 = dz(vz + KVz3) * dt;

		/*������ ������� ���������*/
		x = x + (Kx1 + 2 * Kx2 + 2 * Kx3 + Kx4) / 6;
		y = y + (Ky1 + 2 * Ky2 + 2 * Ky3 + Ky4) / 6;
		z = z + (Kz1 + 2 * Kz2 + 2 * Kz3 + Kz4) / 6;
		R = r(x, y, z);

		double domega_x = d_omega_x(Angle_pitch, Angle_roving, Angle_bank, inc,
				latitude, x, y, z, omega_x, omega_y, omega_z);
		double domega_y = d_omega_y(Angle_pitch, Angle_roving, Angle_bank, inc,
				latitude, x, y, z, omega_x, omega_y, omega_z);
		double domega_z = d_omega_z(Angle_pitch, Angle_roving, Angle_bank, inc,
				latitude, x, y, z, omega_x, omega_y, omega_z);

		double omega_x = omega_x + domega_x * dt;
		double omega_y = omega_y + domega_y * dt;
		double omega_z = omega_z + domega_z * dt;

		double dangle_bank = d_angle_bank(Angle_roving, Angle_bank, omega_x,
				omega_y, omega_z);
		double dangle_roving = d_angle_roving(Angle_roving, Angle_bank, omega_x,
				omega_y, omega_z);
		double dangle_pitch = d_angle_pitch(Angle_roving, Angle_bank, omega_x,
				omega_y, omega_z);

		double Angle_roving = Angle_roving + dangle_bank * dt;
		double Angle_bank = Angle_bank + dangle_roving * dt;
		double Angle_pitch = Angle_pitch + dangle_pitch * dt;

		/*������� �������������� ������ ������*/
		hs = (R - Re) / 1000;

		double tetta_Big0 = gst_f( day_start, mon_start, year_start, day_of_pr,
				mon_of_pr, year_of_pr, hour_of_pr, min_of_pr, sec_of_pr);///////UNUSED
		int t_day = (hour_of_pr * 3600 + min_of_pr * 60 + sec_of_pr) + t;///////UNUSED

		sec_start = sec_start + dt;
		date0 = *localtime(&sec_start);

		/*������� � ����������� ��*/
		double xg = x * cos(tetta_Big0 + OmE * t_day)
				+ y * sin(tetta_Big0 + OmE * t_day);			///////UNUSED
		double yg = -x * sin(tetta_Big0 + OmE * t_day)
				+ y * cos(tetta_Big0 + OmE * t_day);			///////UNUSED
		double zg = z;
		double rg = r(xg, yg, zg);								///////UNUSED
		/*������ �������������� ���������*/
		//		double fis = asin (zg/rg);																													///////UNUSED
		//		double lyams = atan2(yg,xg);																												///////UNUSED

		/*������ �������� �� ������� ����*/
		vx = vx + (KVx1 + 2 * KVx2 + 2 * KVx3 + KVx4) / 6;
		vy = vy + (KVy1 + 2 * KVy2 + 2 * KVy3 + KVy4) / 6;
		vz = vz + (KVz1 + 2 * KVz2 + 2 * KVz3 + KVz4) / 6;
		V = v(vx, vy, vz);

		double Fs = data_in->Param.Fs;

		if (hs <= 600) {
			for (i = 0; i < 10; i++) {
				if ((Fs > IntOfSunlow[i]) && (Fs < IntOfSunlow[i + 1])) {
					for (j = 0; j < 3; j++) {
						CoeffofAtm[j] = interpol(Fs, IntOfSunlow[i],
								IntOfSunlow[i + 1], Coefflow[i][j],
								Coefflow[i + 1][j]);
					}
				}

				a0 = 9.80665;
				a1 = CoeffofAtm[0];
				a2 = CoeffofAtm[1];
				a3 = CoeffofAtm[2];
			}
		} else if ((hs > 600) && (hs < 1500)) {
			for (i = 0; i < 10; i++) {
				if ((Fs > IntOfSunlow[i]) && (Fs < IntOfSunlow[i + 1])) {
					for (j = 0; j < 3; j++) {
						CoeffofAtm[j] = interpol(Fs, IntOfSunlow[i],
								IntOfSunlow[i + 1], CoeffHight[i][j],
								CoeffHight[i + 1][j]);
					}
				}
				a0 = 9.80665;
				a1 = CoeffofAtm[0];
				a2 = CoeffofAtm[1];
				a3 = CoeffofAtm[2];
			}
		} else {
			a0 = 0;
			a1 = 0;
			a2 = 0;
			a3 = 0;
		}
		t = t + dt;

		data.Param.t0 = data_in->Param.t0;
		data.Param.dt = data_in->Param.dt;
		data.Param.Sm = data_in->Param.Sm;
		data.Param.Cx = data_in->Param.Cx;
		data.Param.m = data_in->Param.m;
		data.Param.Fs = data_in->Param.Fs;
		data.Param.oms = oms;
		data.Param.ps = data_in->Param.ps;
		data.Param.es = data_in->Param.es;
		data.Param.bigoms = bigoms;
		data.Param.incs = incs;
		data.Param.omm = omm;
		data.Param.pm = data_in->Param.pm;
		data.Param.em = data_in->Param.em;
		data.Param.bigomm = bigomm;
		data.Param.pm = data_in->Param.pm;
		data.Param.incm = incm;
		data.Param.latitude = latitude;
		data.Param.Angle_pitch = data_in->Param.Angle_pitch;
		data.Param.Angle_roving = data_in->Param.Angle_roving;
		data.Param.Angle_bank = data_in->Param.Angle_bank;
		data.Param.omega_x = data_in->Param.omega_x;
		data.Param.omega_y = data_in->Param.omega_y;
		data.Param.omega_z = data_in->Param.omega_z;

		data.Param.sec = date0.tm_sec;
		data.Param.min = date0.tm_min;
		data.Param.hour = date0.tm_hour;
		data.Param.day = date0.tm_mday;
		data.Param.month = date0.tm_mon + 1;
		data.Param.year = (date0.tm_year + 1900);

		data.a0 = a0;
		data.a1 = a1;
		data.a2 = a2;
		data.a3 = a3;
		data.Param.x = x;
		data.Param.y = y;
		data.Param.z = z;
		data.Param.vx = vx;
		data.Param.vy = vy;
		data.Param.vz = vz;
		data.Param.t = t;



//		printf(" calculate ok!\n");
	}
	data.init = data_in->init;
/*	finish = clock();
	time_pr = ((double) (finish - start)) / CLOCKS_PER_SEC;
	printf(" %lf:)\n", time_pr);
	*/
	/*
	printf("\ndata_out.init = %d\n", data.init);
	printf("\ndata_out.Param.x = %f\n", data.Param.x);

	 */
	//отправка данных
	//int i = 0;
/*
	//printf("\n\ndays: %d\n\n",two_date(1,1,2017,7,1,2017));
	printf("data_out.Param.as = %3f\n", data.as);
	printf("data_out.Param.M0s = %3f\n", data.M0s);
	printf("data_out.Param.am = %3f\n", data.am);
	printf("data_out.Param.M0m = %3f\n", data.M0m);
	printf("data_out.Param.E0s = %3f\n", E0s);
	printf("data_out.Param.k1 = %3f\n", k1);
	printf("data_out.Param.es = %3f\n", data.Param.es);
	printf("data_out.Param.u1s = %3f\n", data.u1s);
	printf("data_out.Param.oms = %3f\n", data.Param.oms);
	printf("data_out.Param.tetta1s = %3f\n", tetta1s);

	printf("data_out.Param.t = %3f\n", data.Param.t);
	printf("data_out.Param.x = %3f\n", data.Param.x);
	printf("data_out.Param.y = %3f\n", data.Param.y);
	printf("data_out.Param.z = %3f\n", data.Param.z);
	printf("data_out.Param.vx = %3f\n", data.Param.vx);
	printf("data_out.Param.vy = %3f\n", data.Param.vy);
	printf("data_out.Param.vz = %3f\n", data.Param.vz);
*/
	///Код пользователя///////////////////////////////////////////////////КОНЕЦ
	return (0);
}
//-----------------------------------------------------------------------------



