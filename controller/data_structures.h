#include "system_features.h"

struct _region_features{
	float arrival_rate;
	float mttf;
};

struct _region{
	char ip_controller[16];
	char ip_balancer[16];
	struct _region_features region_features;
	float probability;
};

/*** TODO: initialize with real provided services ***/
enum vm_operation_type{
	ADD, DELETE, REJ
};

// services provided
enum services {
    SERVICE_1, SERVICE_2, SERVICE_3
};

// STAND_BY has value 0, ACTIVE has value 1, REJUVENATING has value 2, and so on...
enum vm_state {
    STAND_BY, ACTIVE, REJUVENATING
};

// service info for each CN
struct vm_service{
    enum services service;
    volatile enum vm_state state;
    int provided_port;
};

typedef struct _vm_data {
    int socket;
    char ip_address[16];
    int port;
    volatile struct vm_service service_info;
    system_features last_features;
    int last_system_features_stored;
    float mttf;
}vm_data;

struct virtual_machine_operation{
	char ip[16];
	int port;
	enum services service;
	enum vm_operation_type op;
};