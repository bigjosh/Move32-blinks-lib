

bool isAlone(void){
	uint8_t neighbors[TILE_SIDES];
	bool alone = true;
	getNeighborStates(neighbors);
	uint8_t i;
	for(i=0; i<TILE_SIDES; i++){
		if (neighbors[i]){
			alone =  false;
			break;
		}
	}
	return alone;
}


