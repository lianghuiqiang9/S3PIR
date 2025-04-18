#include "utils.h"

void getEntryFromDB(uint64_t* DB, uint32_t index, uint64_t *result, uint32_t EntrySize)
{
#ifdef DEBUG
	uint64_t dummyData = index;
	dummyData <<= 1;
	for (uint32_t l = 0; l < EntrySize / 8; l++)
		result[l] = dummyData + l; 
	return;
#endif

	#ifdef SimLargeServer
		memcpy(result, ((uint8_t*) DB) + index, EntrySize);	
	#else
		memcpy(result, DB + index * (EntrySize / 8), EntrySize); 
	#endif	
};

void initDatabase(uint64_t** DB, uint64_t kLogDBSize, uint64_t kEntrySize){
#ifdef SimLargeServer
	uint64_t DBSizeInUint64 = ((uint64_t) 1 << (kLogDBSize-3)) + kEntrySize;		
#else
	uint64_t DBSizeInUint64 = ((uint64_t) kEntrySize / 8) << kLogDBSize;
	cout<< "DBSizeInUint64: " << DBSizeInUint64 <<endl;
#endif	
	*DB = new uint64_t [DBSizeInUint64];
	ifstream frand("/dev/urandom"); 
	frand.read((char*) *DB, DBSizeInUint64);
	frand.close();
}

uint32_t FindCutoff(uint32_t *prfVals, uint32_t PartNum) {
	uint32_t LowerFilter = 0x80000000 - (1 << 28);
	uint32_t UpperFilter = 0x80000000 + (1 << 28);

	uint32_t LowerCnt = 0, UpperCnt = 0, MiddleCnt = 0;	
	for (uint32_t k = 0; k < PartNum; k++)
  	{
		if (prfVals[k] < LowerFilter)
			LowerCnt++;
		else if (prfVals[k] > UpperFilter)
			UpperCnt++;
		else
		{
			prfVals[MiddleCnt] = prfVals[k];	// move to beginning, ok to overwrite filtered stuff
			MiddleCnt++;
		} 	
	}
	if (LowerCnt >= PartNum / 2 || UpperCnt >= PartNum / 2)
	{
	// cout << "Filtered too much" << endl;
		return 0;	// filtered too many, just give up this hint	

	}

	uint32_t *median = prfVals + PartNum / 2 - LowerCnt;
	nth_element(prfVals, median, prfVals + MiddleCnt);
	uint32_t cutoff = *median;
	*median = 0;
	for (uint32_t k = 0; k < MiddleCnt; k++){
		if (prfVals[k] == cutoff) return 0;
	}
	return cutoff;
}