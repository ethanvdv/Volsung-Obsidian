#include <nodes.h>
#include <shell/shell.h>
#include <sys/slist.h>
#include <kernel.h>

#include <stdlib.h>
#include <stdio.h>

/* Stores the list of nodes in a single linked list */
sys_slist_t node_list;

/* Stores the list of active nodes based on thier coordinates */
int active_nodes[12] = {
	NODE_A_ID, 
	NODE_B_ID, 
	NODE_C_ID, 
	NODE_D_ID, 
	NODE_E_ID, 
	NODE_F_ID, 
	NODE_G_ID, 
	NODE_H_ID
};

void print_activenodes(int16_t rx_rssi[]){
	extern int active_nodes[];
	printk("^%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d~%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d#", rx_rssi[0], rx_rssi[1], rx_rssi[2], rx_rssi[3],  
	rx_rssi[4], rx_rssi[5], rx_rssi[6], rx_rssi[7], rx_rssi[8], rx_rssi[9], rx_rssi[10], rx_rssi[11], active_nodes[0], active_nodes[1], 
		active_nodes[2], active_nodes[3], active_nodes[4], active_nodes[5],  
		active_nodes[6], active_nodes[7], active_nodes[8], active_nodes[9], active_nodes[10], active_nodes[11]);
	// printk("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", active_nodes[0], active_nodes[1], 
	// 	active_nodes[2], active_nodes[3], active_nodes[4], active_nodes[5],  
	// 	active_nodes[6], active_nodes[7], active_nodes[8], active_nodes[9], active_nodes[10], active_nodes[11]);
	// for(int j = 0; j < 12; j++){
	// 	printk("%d,",active_nodes[j]);
	// }
}

/* Stores the linked list nodes with additional data */
struct data_node {
	sys_snode_t node;
	int* data;
	int* x;
	int* y;
};

/* Stores the static information of a nodes */
typedef struct Node_Struct {
	char* name;
	char* address;
	int major_num;
	int minor_num;
} Node_Struct;

/* Stores the information of all 12 nodes */
Node_Struct node_all[12] = {
	{"4011-A", "F5:75:FE:85:34:67", 2753, 32998},
	{"4011-B", "E5:73:87:06:1E:86", 32975, 20959},
	{"4011-C", "CA:99:9E:FD:98:B1", 26679, 40363},
	{"4011-D", "CB:1B:89:82:FF:FE", 41747, 38800},
	{"4011-E", "D4:D2:A0:A4:5C:AC", 30679, 51963},
	{"4011-F", "C1:13:27:E9:B7:7C", 6195, 18394},
	{"4011-G", "F1:04:48:06:39:A0", 30525, 30544},
	{"4011-H", "CA:0C:E0:DB:CE:60", 57395, 28931},
	{"4011-I", "D4:7F:D4:7C:20:13", 60345, 49995},
	{"4011-J", "F7:0B:21:F1:C8:E1", 12249, 30916},
	{"4011-K", "FD:E0:8D:FA:3E:4A", 36748, 11457},
	{"4011-L", "EE:32:F7:28:FA:AC", 27564, 27589}
};

/**
 * @brief Update active_node array based on given nodes' coordinates
 */
void update_active_nodes(void) {
	for (int i = 1; i <= 12; i++) {
		switch(get_node_x(i)) {
			case 0:
				switch(get_node_y(i)) {
					case 0:
						active_nodes[NODE_POSITION_ONE] = i;
						break;
					case 2:
						active_nodes[NODE_POSITION_EIGHT] = i;
						break;
					case 4:
						active_nodes[NODE_POSITION_SEVEN] = i;
						break;
					default:
						printk("Error at node update\n");
				}
				break;
			case 2:
				switch(get_node_y(i)) {
					case 0:
						active_nodes[NODE_POSITION_TWO] = i;
						break;
					case 4:
						active_nodes[NODE_POSITION_SIX] = i;
						break;
					default:
						printk("Error at node update\n");
				}
				break;
			case 4:
				switch(get_node_y(i)) {
					case 0:
						active_nodes[NODE_POSITION_THREE] = i;
						break;
					case 2:
						active_nodes[NODE_POSITION_FOUR] = i;
						break;
					case 4:
						active_nodes[NODE_POSITION_FIVE] = i;
						break;
					default:
						printk("Error at node update\n");
				}
		}
	}
}

/**
 * @brief Takes a node as the target and find the next node in the list
 * 
 * @param target : the target node to check for
 * 
 * @returns : next node in the list
 */
sys_snode_t* get_next_node(sys_snode_t* target) {
	if (target == sys_slist_peek_tail(&node_list)) {
		return sys_slist_peek_head(&node_list);
	}
	return sys_slist_peek_next(target);
}

/**
 * @brief Takes a node as the target and find the last node in the list
 * 
 * @param target : the target node to check for
 * 
 * @returns : last node in the list
 */
sys_snode_t* get_last_node(sys_snode_t* target) {
	sys_snode_t* head_node = sys_slist_peek_head(&node_list);
	sys_snode_t* tail_node = sys_slist_peek_tail(&node_list);
	sys_snode_t* last_node = NULL;
	if (head_node == tail_node) {
		return head_node;
	}
	if (target == head_node) {
		return tail_node;
	} 
	while (true) {
		last_node = head_node;
		head_node = sys_slist_peek_next(head_node);
		if (target == head_node) {
			return last_node;
		}
		if (head_node == sys_slist_peek_tail(&node_list)) {
			return NULL;
		}
	}
}

/**
 * @brief Takes a node id and attempt to fond the node
 * 
 * @param node_id : the node id to search for
 * 
 * @returns : node id's node
 */
sys_snode_t* find_node(int node_id) {
	if (node_id < 1 || node_id > 12) {
		return 0;
	}
	sys_snode_t* node = sys_slist_peek_head(&node_list);
	while (true) {
		int node_number = *((struct data_node *)node)->data;
		if (node_id == node_number) {
			return node;
		}
		if (node != sys_slist_peek_tail(&node_list)) {
			node = sys_slist_peek_next(node);
		} else {
			return 0;
		}
	}
	return 0;
}

/**
 * @brief Initialise a data_node struct allocating memories
 * 
 * @returns : the initialised data_node struct
 */
struct data_node* data_node_init(void) {
	struct data_node* node;
	node = (struct data_node*)k_malloc(sizeof(struct data_node));
	node->data = (int*)k_malloc(sizeof(int));
	node->x = (int*)k_malloc(sizeof(int));
	node->y = (int*)k_malloc(sizeof(int));
	return node;
}

/**
 * @brief Free a data_node struct's memories
 * 
 * @param node : the struct to free
 */
void data_node_free(sys_snode_t* node) {
	k_free(((struct data_node*)node)->data);
	k_free(((struct data_node*)node)->x);
	k_free(((struct data_node*)node)->y);
	k_free(node);
}

/**
 * @brief Takes a node id and print all the node infos without neighbour infos
 * 
 * @param node_id : the node id to check for
 */
void print_node_info(int node_id) {
	sys_snode_t* node = find_node(node_id);
	int x = *((struct data_node*)node)->x;
	int y = *((struct data_node*)node)->y;
	printk("Node ID: %d has the following properties:\n", node_id);
	printk("Node name: %s\n", node_all[node_id-1].name);
	printk("Node MAC Address: %s\n", node_all[node_id-1].address);
	printk("Node major number: %d\n", node_all[node_id-1].major_num);
	printk("Node minor number: %d\n", node_all[node_id-1].minor_num);
	printk("Node X coordinate: %d\n", x);
	printk("Node Y coordinate: %d\n", y);
}

/**
 * @brief Print the node neightbour information into the shell. Left being
 * 		  the node before the target on the linked list and right being the
 * 		  node after the target 
 * 
 * @param node_id : the id of the node to check for neighbours
 */
void print_node_neighbours(int node_id) {
	sys_snode_t* node = find_node(node_id);
	sys_snode_t* next_node = get_next_node(node);
	sys_snode_t* last_node = get_last_node(node);
	int next_id = *((struct data_node*)next_node)->data;
	int last_id = *((struct data_node*)last_node)->data;
	printk("Node left neighbour: %s\n", node_all[last_id-1].name);
	printk("Node right neighbour: %s\n", node_all[next_id-1].name);
}

/**
 * @brief Takes the data, x and y values to create a new node
 * 
 * @param data : the data passed to the data_node struct
 * @param x : the x passed to the data_node struct
 * @param y : the y passed to the data_node struct
 * 
 * @returns : next node in the list
 */
void add_node(int data, int x, int y) {
	struct data_node* data_node = data_node_init();
	*data_node->data = data;
	*data_node->x = x;
	*data_node->y = y;
	sys_slist_append(&node_list, &data_node->node);
	update_active_nodes();
}

/**
 * @brief handler for add new nodes function
 * 
 * @param sh : the shell
 * @param argc : number of arguments
 * @param argv : the arguments
 * 
 * @returns : 0 on success
 */
static int add_node_handler(const struct shell *sh, size_t argc, char **argv) {
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int node_number = atoi(argv[1]);
	int node_x = atoi(argv[2]);
	int node_y = atoi(argv[3]);
	if (node_number < 1 || node_number > 12) {
		printk("Invalid node id\n");
		return 0;
	}
	if (find_node(node_number)) {
		printk("Node already exist\n");
		return 0;
	}
	add_node(node_number, node_x, node_y);
	printk("Added the following node\n");
	print_node_info(node_number);
	return 0;
}

/**
 * @brief handler for remove nodes function
 * 
 * @param sh : the shell
 * @param argc : number of arguments
 * @param argv : the arguments
 * 
 * @returns : 0 on success
 */
static int remove_node_handler(const struct shell *sh, size_t argc, char **argv) {
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int node_number = atoi(argv[1]);
	sys_snode_t* node;
	if ((node = find_node(node_number))) {
		sys_slist_find_and_remove(&node_list, node);
		data_node_free(node);
		printk("Node %d removed\n", node_number);
		return 0;
	} else {
		printk("Node doesn't exist\n");
	}
	return 0;
}

/**
 * @brief handler for displaying nodes function. display one or all nodes
 * 
 * @param sh : the shell
 * @param argc : number of arguments
 * @param argv : the arguments
 * 
 * @returns : 0 on success
 */
static int get_node_handler(const struct shell *sh, size_t argc, char **argv) {
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	char* node_name = argv[1];
    if (sys_slist_is_empty(&node_list)) {
        printk("Node list is empty\n");
    } else {
		if (strcmp(node_name, "all") == 0) {
			sys_snode_t* node = sys_slist_peek_head(&node_list);
			while (true) {
				int node_number = *((struct data_node*)node)->data;
				print_node_info(node_number);
				print_node_neighbours(node_number);
				if (node != sys_slist_peek_tail(&node_list)) {
					node = sys_slist_peek_next(node);
				} else {
					break;
				}
			}
		} else {
			int node_id = atoi(node_name);
			if (!find_node(node_id)) {
				printk("No such node was found\n");
			} else {
				print_node_info(node_id);
				print_node_neighbours(node_id);
				for (int i = 0; i < 8; i++) {
					printk("Active Nodes is %d\n", active_nodes[i]);
				}
			}
		} 
    }
	return 0;
}

/**
 * @brief Initialise all the default nodes and their positions (A - H)
 */
void default_nodes_init(void) {
	add_node(NODE_A_ID, 0, 0);
	add_node(NODE_B_ID, 2, 0);
	add_node(NODE_C_ID, 4, 0);
	add_node(NODE_D_ID, 4, 2);
	add_node(NODE_E_ID, 4, 4);
	add_node(NODE_F_ID, 2, 4);
	add_node(NODE_G_ID, 0, 4);
	add_node(NODE_H_ID, 0, 2);
}

/**
 * @brief Initialise shell commands and node list and default nodes
 */
void node_list_cmd_init(void) {
    sys_slist_init(&node_list);
	default_nodes_init();
    SHELL_STATIC_SUBCMD_SET_CREATE(
        node_handler,
        SHELL_CMD_ARG(add, NULL,
            "Add nodes to the list of nodes. command: node add [id] [x] [y]",
            &add_node_handler, 4, 0
        ),
		SHELL_CMD_ARG(remove, NULL,
			"Remove nodes from the list of nodes. command: node remove [id]",
			&remove_node_handler, 2, 0
		),
        SHELL_CMD_ARG(get, NULL,
			"Get node or nodes from the list of nodes. command: node get [id]",
			&get_node_handler, 2, 0
		),
        SHELL_SUBCMD_SET_END
    );
    SHELL_CMD_REGISTER(node, &node_handler,
		"Set node information", NULL
	);
}

/**
 * @brief Get the name of the node with node id
 * 
 * @return : name of the node
 */
char* get_node_name(int node_id) {
	return node_all[node_id-1].name;
}

/**
 * @brief Get the mac address of the node with node id
 * 
 * @return : mac address of the node
 */
char* get_node_mac(int node_id) {
	return node_all[node_id-1].address;
}

/**
 * @brief Get the x position of the node with node id
 * 
 * @return : x position  of the node
 */
int get_node_x(int node_id) {
	sys_snode_t* node = find_node(node_id);
	if (node == 0) {
		return -1;
	}
	return *((struct data_node*)node)->x;
}

/**
 * @brief Get the y position of the node with node id
 * 
 * @return : y position  of the node
 */
int get_node_y(int node_id) {
	sys_snode_t* node = find_node(node_id);
	if (node == 0) {
		return -1;
	}
	return *((struct data_node*)node)->y;
}