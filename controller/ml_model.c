#include "ml_models.h"

#include <stdio.h>


#define ALPHA 0.6


// This computes the prediction model for linear regression
float calculate_linear_regression(system_features_with_slopes f) {

	//struct sysinfo info;
	//sysinfo(&info);

	//unsigned long free_ram = info.totalram;

	//printf("*******************Total memory in bytes: %lu\n", total_ram);

	return
//    -566.944 *  f.gen_time +
    -80.1021 *  f.n_th_slope +
    -15.8269 *  f.mem_buffers_slope +
     59.5421 *  f.cpu_user_slope +
    -46.2345 *  f.cpu_steal_slope +
    -46.51   *  f.cpu_idle_slope +
      7.6227 *  f.n_th +
     -0.0012 *  f.mem_used +
     -0.0029 *  f.mem_buffers +
      0.0002 *  f.mem_cached +
     71.333  *  f.cpu_user +
     72.2214 *  f.cpu_nice +
    -36.6341 *  f.cpu_system +
    -21.3804 *  f.cpu_steal +
     23.6648 *  f.cpu_idle +
   -352.1951;
}

// This evaluates the MTTF
float get_predicted_mttf(int ml_model, system_features last_features, system_features current_features, system_features init_features) {

	static float mem_used_slope = 0.0;

	system_features_with_slopes f;
	f.gen_time=current_features.time-last_features.time;
	f.n_th_slope=current_features.n_th-last_features.n_th;
	f.mem_used_slope=current_features.mem_used-last_features.mem_used;
	f.mem_free_slope=current_features.mem_free-last_features.mem_free;
	f.mem_buffers_slope=current_features.mem_buffers-last_features.mem_buffers;
	f.swap_used_slope=current_features.swap_used-last_features.swap_used;
	f.swap_free_slope=current_features.swap_free-last_features.swap_free;
	f.cpu_user_slope=current_features.cpu_user-last_features.cpu_user;
	f.cpu_system_slope=current_features.cpu_system-last_features.cpu_system;
	f.cpu_idle_slope=current_features.cpu_idle-last_features.cpu_idle;
	f.cpu_steal_slope=current_features.cpu_steal-last_features.cpu_steal;
	f.n_th=init_features.n_th;
	f.mem_free=last_features.mem_free;
	f.mem_used=init_features.mem_used;
	f.mem_buffers=init_features.mem_buffers;
	f.swap_used=init_features.swap_used;
	f.cpu_user=init_features.cpu_user;
	f.cpu_nice=init_features.cpu_nice;
	f.cpu_system=init_features.cpu_system;
	f.cpu_iowait=init_features.cpu_iowait;
        f.cpu_idle=init_features.cpu_idle;

	if(f.mem_used_slope > 0.0) {
//		if(mem_used_slope == 0.0) {
//			mem_used_slope = f.mem_used_slope;
//		} else {
			mem_used_slope = ALPHA * mem_used_slope + (1 - ALPHA) * f.mem_used_slope;
//		}
	}

    //return calculate_linear_regression(f);
        float predicted=(float)((float)init_features.mem_free/mem_used_slope)*f.gen_time;
        printf("Initial memory: %d, mem_used_slope: %f, moving_avg: %f, predicted mttf %f\n", init_features.mem_free, f.mem_used_slope/f.gen_time, mem_used_slope/f.gen_time, predicted);
        return predicted;

}

// This evaluates the RTTF
float get_predicted_rttc(int ml_model, system_features last_features, system_features current_features) {

	static float mem_used_slope = 0.0;

	system_features_with_slopes f;
	f.gen_time=current_features.time-last_features.time;
	f.n_th_slope=current_features.n_th-last_features.n_th;
	f.mem_used_slope=current_features.mem_used-last_features.mem_used;
	f.mem_free_slope=current_features.mem_free-last_features.mem_free;
	f.mem_buffers_slope=current_features.mem_buffers-last_features.mem_buffers;
	f.swap_used_slope=current_features.swap_used-last_features.swap_used;
	f.swap_free_slope=current_features.swap_free-last_features.swap_free;
	f.cpu_user_slope=current_features.cpu_user-last_features.cpu_user;
	f.cpu_system_slope=current_features.cpu_system-last_features.cpu_system;
	f.cpu_idle_slope=current_features.cpu_idle-last_features.cpu_idle;
	f.cpu_steal_slope=current_features.cpu_steal-last_features.cpu_steal;
	f.n_th=last_features.n_th;
	f.mem_free=last_features.mem_free;
	f.mem_used=last_features.mem_used;
	f.mem_buffers=last_features.mem_buffers;
	f.swap_used=last_features.swap_used;
	f.cpu_user=last_features.cpu_user;
	f.cpu_nice=last_features.cpu_nice;
	f.cpu_system=last_features.cpu_system;
	f.cpu_iowait=last_features.cpu_iowait;
        f.cpu_idle=last_features.cpu_idle;

	if(f.mem_used_slope > 0.0) {
  //              if(mem_used_slope == 0.0) {
    //                    mem_used_slope = f.mem_used_slope;
      //          } else {
                        mem_used_slope = ALPHA * mem_used_slope + (1 - ALPHA) * f.mem_used_slope;
        //        }
        }

	float predicted=(float)((float)f.mem_free/mem_used_slope)*f.gen_time;
        printf("Free memory: %d, mem_used_slope: %f, moving_avg: %f, predicted rttf: %f\n", f.mem_free, f.mem_used_slope/f.gen_time, mem_used_slope/f.gen_time, predicted);
        return predicted;
}
