#include <cassert>

#include "server.h"
#include "utils.h"

OneSVServer::OneSVServer(uint64_t * DB_ptr, uint32_t LogN, uint32_t EntryB){
  assert(LogN < 32);
  assert(EntryB >= 8);
  N = 1 << LogN;
  B = EntryB / 8;
	EntrySize = EntryB;
  DB = DB_ptr;
	PartNum = 1 << (LogN / 2);
	PartSize = 1 << (LogN / 2 + LogN % 2);
	lambda = LAMBDA;
	M = lambda * PartSize;
  tmpEntry = new uint64_t[B];
}

void OneSVServer::getEntry(uint32_t index, uint64_t *result){
  getEntryFromDB(DB, index, result, EntrySize);
}

void OneSVServer::onlineQuery(bool* bvec, uint32_t *Svec, uint64_t *b0, uint64_t *b1){
	for (uint32_t k = 0; k < PartNum; k++)
	{
		getEntryFromDB(DB, k * PartSize + Svec[k], tmpEntry, EntrySize);
		if (bvec[k])
			for (uint32_t l = 0; l < B; l++)
				b1[l] ^= tmpEntry[l];
		else
			for (uint32_t l = 0; l < B; l++)
				b0[l] ^= tmpEntry[l];
	}
}

TwoSVServer::TwoSVServer(uint64_t * DB_ptr, uint32_t LogN, uint32_t EntryB): 
 OneSVServer(DB_ptr, LogN, EntryB),
 prf(AES_KEY){}

void TwoSVServer::replenishHint(uint64_t hintID, uint64_t * result, uint32_t * SelectCutoff){
	
	// Run server side part of Algorithm 3.
	memset(result, 0, 2*B*sizeof(uint64_t));
	uint64_t entry[B]; 
	uint16_t prfIndices[8];
	uint32_t prfSelectVals[PartNum];
	
	for (uint32_t k = 0; k < PartNum; k+=4){
		prf.evaluate((uint8_t*) (prfSelectVals + k), hintID, k/4, 1);
	}

	// Get median of selectvals
	uint32_t prfSelectValsCopy[PartNum];
	memcpy(prfSelectValsCopy, prfSelectVals, PartNum*sizeof(uint32_t));
	*SelectCutoff = FindCutoff(prfSelectValsCopy, PartNum);
	
	for (uint32_t k = 0; k < PartNum; k++){
		if (k % 8 == 0){
			prf.evaluate((uint8_t*)prfIndices, hintID, k / 8, 2);
		}
		bool b = prfSelectVals[k] < *SelectCutoff;
		uint16_t idx = prfIndices[k%8] & (PartSize - 1);
		getEntry(k*PartSize + idx, entry);
		

		if (b){
			for (uint32_t l = 0; l < B; l++){
				result[l + B] ^= entry[l]; 
			}
		} else {
			for (uint32_t l = 0; l < B; l++){
				result[l] ^= entry[l]; 
			}
		}
	}
}

void TwoSVServer::generateOfflineHints(uint32_t M, uint64_t * Parity, uint16_t * ExtraPart, uint16_t * ExtraOffset, uint32_t * SelectCutoff ){

	// Run Algorithm 1.
	uint16_t prfIndices [8];
	uint32_t prfSelectVals[PartNum];
	uint32_t InvalidHints = 0;
	// Compute our hints
	for (uint32_t hint_number = 0; hint_number < M; hint_number++)
	{
		for (uint32_t k = 0; k < PartNum; k += 4){
			prf.evaluate((uint8_t*) (prfSelectVals + k), hint_number, k / 4, 1);
		}

		uint32_t prfSelectValsCopy[PartNum];

		memcpy(prfSelectValsCopy, prfSelectVals, PartNum * sizeof(uint32_t));
		uint32_t cutoff = FindCutoff(prfSelectValsCopy, PartNum);
		InvalidHints += !cutoff;
		SelectCutoff[hint_number] = cutoff;

		// Choose extra index
		uint16_t ePart;
		bool b = 1;
		while (b) {
			ePart = NextDummyIdx() % PartNum;
			b = prfSelectVals[ePart] < cutoff;
		}
		uint16_t eIdx = NextDummyIdx() % PartSize;
		ExtraPart[hint_number] = ePart;
		ExtraOffset[hint_number] = eIdx;
		getEntry(ePart*PartSize + eIdx, Parity + hint_number*B);
		
		for (uint32_t part_number = 0; part_number < PartNum; part_number++) {
			if (part_number % 8 == 0){
				prf.evaluate((uint8_t*) prfIndices, hint_number, part_number / 8, 2);
			}
			if (prfSelectVals[part_number] < cutoff){
				getEntry((prfIndices[part_number % 8] & (PartSize - 1)) + part_number * PartSize, tmpEntry);
				for (uint32_t l = 0; l < B; l++)
					Parity[hint_number*B+l] ^= tmpEntry[l];
			}
		}
	}
	cout << "Invalid hints: " << InvalidHints << endl;
}

uint16_t TwoSVServer::NextDummyIdx()
{
	if (dummyIdxUsed % 8 == 0)	// need more dummy indices
		prf.evaluate((uint8_t*) prfDummyIndices, 0, dummyIdxUsed / 8, 0);
	return prfDummyIndices[dummyIdxUsed++ % 8]; 
}