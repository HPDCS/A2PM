#include "ml_models.h"

// This computes the prediction model for linear regression
float calculate_linear_regression(system_features_with_slopes f) {
	return
    -566.944  *  GenTime +
    -80.1021 *  n_th_slope +
    -15.8269 *  mem_buffers_slope +
    59.5421 *  cpu_user_slope +
    -46.2345 *  cpu_steal_slope +
    -46.51   *  cpu_idle_slope +
    7.6227 *  n_th +
    -0.0012 *  mem_used +
    -0.0029 *  mem_buffers +
    0.0002 *  mem_cached +
    71.333  *  cpu_user +
    72.2214 *  cpu_nice +
    -36.6341 *  cpu_system +
    -21.3804 *  cpu_steal +
    23.6648 *  cpu_idle +
    -352.1951
}


// This evaluates the MTTF
float get_predicted_mttf(int ml_model, system_features last_features, system_features current_features, system_features init_features){

	system_features_with_slopes f;
	f.gen_time=current_features.time-last_features.time;
	f.n_th_slope=current_features.n_th-last_features.n_th;
	f.mem_used_slope=current_features.mem_used-last_features.mem_used;
	f.mem_free_slope=current_features.mem_free-last_features.mem_free;
	f.swap_used_slope=current_features.swap_used-last_features.swap_used;
	f.swap_free_slope=current_features.swap_free-last_features.swap_free;
	f.cpu_user_slope=current_features.cpu_user-last_features.cpu_user;
	f.cpu_system_slope=current_features.cpu_system-last_features.cpu_system;
	f.cpu_idle_slope=current_features.cpu_idle-last_features.cpu_idle;
	f.n_th=init_features.n_th;
	f.mem_used=init_features.mem_used;
	f.mem_buffers=init_features.mem_buffers;
	f.swap_used=init_features.swap_used;
	f.cpu_user=init_features.cpu_user;
	f.cpu_nice=init_features.cpu_nice;
	f.cpu_system=init_features.cpu_system;
	f.cpu_iowait=init_features.cpu_iowait;
    f.cpu_idle=init_features.cpu_idle;

    return calculate_linear_regression(f);
}

// This evaluates the RTTF
float get_predicted_rttc(int ml_model, system_features last_features, system_features current_features) {

	system_features_with_slopes f;
	f.gen_time=current_features.time-last_features.time;
	f.n_th_slope=current_features.n_th-last_features.n_th;
	f.mem_used_slope=current_features.mem_used-last_features.mem_used;
	f.mem_free_slope=current_features.mem_free-last_features.mem_free;
	f.swap_used_slope=current_features.swap_used-last_features.swap_used;
	f.swap_free_slope=current_features.swap_free-last_features.swap_free;
	f.cpu_user_slope=current_features.cpu_user-last_features.cpu_user;
	f.cpu_system_slope=current_features.cpu_system-last_features.cpu_system;
	f.cpu_idle_slope=current_features.cpu_idle-last_features.cpu_idle;
	f.n_th=last_features.n_th;
	f.mem_used=last_features.mem_used;
	f.mem_buffers=last_features.mem_buffers;
	f.swap_used=last_features.swap_used;
	f.cpu_user=last_features.cpu_user;
	f.cpu_nice=last_features.cpu_nice;
	f.cpu_system=last_features.cpu_system;
	f.cpu_iowait=last_features.cpu_iowait;
    f.cpu_idle=last_features.cpu_idle;

    return calculate_linear_regression(f);

/*
	switch (ml_model) {
	case LASSO_MODEL:

		break;
	case LINEAR_REGRESSION_MODEL:
		break;
	case M5P_MODEL:
		break;
	case REPTREE_MODEL:
		break;
	case SVM_MODEL:
		break;
	case SVM2_MODEL:
		break;
	default:
		printf ("\nError: No machine learning model selected");
		break;
	}
	*/
}
