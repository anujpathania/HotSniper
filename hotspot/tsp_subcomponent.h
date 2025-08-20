#ifndef __TSP_SUBCOMPONENT_H_
#define __TSP_SUBCOMPONENT_H_

/* floorplan component model */
typedef struct flp_unit_t {
	char* name;

	/* dimensions of the component */
	double width;
	double height;
	double left;
	double bottom;
}flp_unit_t;

/* block thermal model	*/
typedef struct tsp_model_t_st
{
	/* floorplan	*/
	flp_unit_t *flp;

	/* main matrices	*/
	/* conductance matrix */
	double **b;
	double **invb;
	double *g_amb;
	/* block power */
	double *p;

	int *mapping;

	double t_amb;
	double t_dtm;
	double p_max;
	double p_inactive_core;

	/* total no. of nodes	*/
	int n_nodes;
	/* total no. of components	*/
	int n_components;
}tsp_model_t;

/*
hotspot.c:475 << calculate temperature values and write them to instantaneousTempFile.
scheduler.open.cc << performance counters read from instantaneousTempFile to get temperature of component/core
*/

#endif
