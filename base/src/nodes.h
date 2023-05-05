#ifndef NODES_H
#define NODES_H

#include <sys/slist.h>

/* Node ids */
#define NODE_A_ID 1
#define NODE_B_ID 2
#define NODE_C_ID 3
#define NODE_D_ID 4
#define NODE_E_ID 5
#define NODE_F_ID 6
#define NODE_G_ID 7
#define NODE_H_ID 8
#define NODE_I_ID 9
#define NODE_J_ID 10
#define NODE_K_ID 11
#define NODE_L_ID 12

/* Active nodes postions */
#define NODE_POSITION_ONE 		0
#define NODE_POSITION_TWO 		1
#define NODE_POSITION_THREE		2
#define NODE_POSITION_FOUR		3
#define NODE_POSITION_FIVE		4
#define NODE_POSITION_SIX		5
#define NODE_POSITION_SEVEN		6
#define NODE_POSITION_EIGHT		7

void print_activenodes(int16_t[]);

void node_list_cmd_init(void);

char* get_node_name(int);

char* get_node_mac(int);

int get_node_x(int);

int get_node_y(int);

#endif