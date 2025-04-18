#include "client.h"
#include "server.h"
#include <random>
#include <algorithm>
#include <cassert>

using namespace std;
using namespace CryptoPP;

// N is supported up to 2^32. Allows us to use uint16_t to store a single offset within partition
template <class PRF>
Client<PRF>::Client(uint32_t LogN, uint32_t EntryB):
	prf(AES_KEY) 
{
	assert(LogN < 32);
	assert(EntryB >= 8);
	N = 1 << LogN;
	// B is the size of one entry in uint64s
	B = EntryB / 8;

	PartNum = 1 << (LogN / 2);
	PartSize = 1 << (LogN / 2 + LogN % 2);
	lambda = LAMBDA;
	M = lambda * PartSize;

	// Allocate memory for making requests to servers and receiving responses from servers
	bvec = new bool [PartNum];
	Svec = new uint32_t [PartNum];
	Response_b0 = new uint64_t [B];
	Response_b1 = new uint64_t [B];
	tmpEntry = new uint64_t [B];

	// Allocate storage for hints
	HintID = new uint32_t [M];
	ExtraPart = new uint16_t [M];
	ExtraOffset = new uint16_t [M];

	// Pack the bits together
	IndicatorBit = new uint8_t[(M+7)/8];
	LastHintID = 0;

	dummyIdxUsed = 0;			// prfDummyIndices should be accessed modulo 8
}

template <class PRF>
uint16_t Client<PRF>::NextDummyIdx() {
	if (dummyIdxUsed % 8 == 0)	// need more dummy indices
		prf.evaluate((uint8_t*) prfDummyIndices, 0, dummyIdxUsed / 8, 0);
	return prfDummyIndices[dummyIdxUsed++ % 8]; 
}

template <class PRF>
uint64_t Client<PRF>::find_hint(uint32_t query, uint16_t queryPartNum, uint16_t queryOffset, bool &b_indicator){
	for (uint64_t hintIndex = 0; hintIndex < M; hintIndex++){
		if (SelectCutoff[hintIndex] == 0){ // Invalid hint
			continue;
		}
		b_indicator = (IndicatorBit[hintIndex/8] >> (hintIndex % 8)) & 1;
		if (ExtraPart[hintIndex] == queryPartNum && ExtraOffset[hintIndex] == queryOffset){ // Query is the extra entry that the hint stores
			return hintIndex;
		}
		uint32_t r = prf.PRF4Idx(HintID[hintIndex], queryPartNum);	
		if ((r ^ query) & (PartSize-1)){ // Check if r == query mod PartSize
			continue;
		}
		bool b = prf.PRF4Select(HintID[hintIndex], queryPartNum, SelectCutoff[hintIndex]);	
		if (b == b_indicator){
			return hintIndex;
		}
	}
	return M+1;
}


TwoSVClient::TwoSVClient(uint32_t LogN, uint32_t EntryB):
	Client(LogN, EntryB)
{
	SelectCutoff = new uint32_t [M];
	Parity = new uint64_t [M*B];
}

void TwoSVClient::Offline(TwoSVServer & offline_server) {
	// Initialize the hint parity array to 0.
	memset(Parity, 0, sizeof(uint64_t) * B * M);
	// For offline generation the indicator bit is always set to 1.
	memset(IndicatorBit, 255, (M+7)/8);

  // Initialize hints.
	for (uint64_t j = 0; j < M; j++){
		HintID[j] = j;
	}

	offline_server.generateOfflineHints(M,Parity,ExtraPart,ExtraOffset, SelectCutoff);
	LastHintID = M;
}


void TwoSVClient::Online(TwoSVServer & online_server, TwoSVServer & offline_server, uint32_t query, uint64_t *result)
{
	assert(query <= N);
	uint16_t queryPartNum = query / PartSize;
	uint16_t queryOffset = query & (PartSize-1);
	bool b_indicator = 0; // Indicator bit of the selected hint. 

	// Run Algorithm 2
  	// Find a hint that has our desired query index
	//找到query位于大那个hint中
	uint64_t hintIndex = find_hint(query, queryPartNum, queryOffset, b_indicator);
	assert(hintIndex < M);

	// Build a query. 
	uint32_t hintID = HintID[hintIndex];
	// Buffer the hint indices and select values to reduce number of calls to PRF.
	uint16_t prfIndices [8]; 
	uint32_t prfSelectVals[4];
	// Randomize the selector bit sent to the server
	bool shouldFlip = rand() & 1;
	uint32_t cutoff = SelectCutoff[hintIndex];

	for (uint32_t part_i = 0; part_i < PartNum; part_i++) {
		// Each prf evaluation generates the in-partition offsets for 8 consecutive partitions
		if (part_i % 8 == 0) {
			prf.PRFBatchIdx(prfIndices, hintID, part_i);
		}
		// Each prf evaluation generates the v values for 4 consecutive partitions
		if (part_i % 4 == 0) {
			prf.PRFBatchSelect(prfSelectVals, hintID, part_i);
		}

		if (part_i == queryPartNum) {
			// Current partition is the queried partition of interest
			bvec[part_i] = (!b_indicator) ^ shouldFlip;	// Assign to dummy query
			Svec[part_i] = NextDummyIdx() & (PartSize-1);
			continue; 
		} else if (ExtraPart[hintIndex] == part_i) {
			// Current partition is the hint's extra partition
			bvec[part_i] = b_indicator ^ shouldFlip;	// Assign to real query
			Svec[part_i] = ExtraOffset[hintIndex];
			continue;
		}	

		bool b = prfSelectVals[part_i % 4] < cutoff;
		bvec[part_i] = b ^ shouldFlip;
		if (b == b_indicator) {
			// Assign part to real query
			Svec[part_i] = prfIndices[part_i % 8] & (PartSize-1);
		} else {
			// Assign part to dummy query
			Svec[part_i] = NextDummyIdx() & (PartSize-1);	
		}
	}

	// Make our query
	memset(Response_b0, 0, sizeof(uint64_t) * B);
	memset(Response_b1, 0, sizeof(uint64_t) * B);
	online_server.onlineQuery(bvec, Svec, Response_b0, Response_b1);
	
	// Set the query result to the correct response.
	uint64_t * QueryResult = (!b_indicator ^ shouldFlip) ? Response_b0 : Response_b1;

	for (uint32_t l = 0; l < B; l++) {
		result[l] = QueryResult[l] ^ Parity[hintIndex*B + l]; 
	}

	#ifdef DEBUG
	// Check actual value 
	online_server.getEntry(query, tmpEntry);

	for (uint32_t l = 0; l < B; l++){
		if (result[l] != tmpEntry[l]) {
			cout << "Query failed, debugging query " << query << endl;
			assert(query < N);
			cout << "Computed result: " << result[l] << "\n" << "Actual entry value: " << tmpEntry[l] << endl;
			cout << "Parity sent by server: " << QueryResult[l] << "\nHint parity: " << Parity[hintIndex*B + l] << endl;
			for (uint32_t k = 0; k < PartNum; k++)
			{
				if (query / PartSize != k && ExtraPart[hintIndex] != k)
				{
					assert(prf.PRF4Select(hintID, k, cutoff) == bvec[k] ^ shouldFlip);
					assert(!(bvec[k] ^ shouldFlip) || prf.PRF4Idx(hintID, k) % PartSize == Svec[k]);
				}
			}
			cout << "Extra entry partition num: " << ExtraPart[hintIndex] << "\nExtra entry offset: " << ExtraOffset[hintIndex] << endl;
			assert(0);
		}
	}
	#endif

	// Run client side part of Algorithm 3.
  // Replenish hint. Parity indicator represents the bit that we will use for our hint.
	uint64_t hint_parities[2*B];
	++LastHintID;
	offline_server.replenishHint(LastHintID, hint_parities, SelectCutoff + hintIndex);

	b_indicator = !(prf.PRF4Select(LastHintID, queryPartNum, SelectCutoff[hintIndex]));
	IndicatorBit[hintIndex/8] = (IndicatorBit[hintIndex/8] & ~(1<<(hintIndex%8))) | b_indicator << (hintIndex % 8);
	HintID[hintIndex] = LastHintID;
	ExtraPart[hintIndex] = queryPartNum;
	ExtraOffset[hintIndex] = queryOffset; 
	for (uint32_t l = 0; l < B; l++){
		Parity[hintIndex*B+l] = hint_parities[b_indicator*B+l] ^ result[l];
	}
}

OneSVClient::OneSVClient(uint32_t LogN, uint32_t EntryB):
	Client(LogN, EntryB)
{
	// Allocate storage for hints. One server version stores M more hint parities and cutoffs as backup hints.
	SelectCutoff = new uint32_t [M*2];	
	Parity = new uint64_t [M*2*B];

	prfSelectVals = new uint32_t [PartNum*4];
	DBPart = new uint64_t [PartSize * B]; 
}


void OneSVClient::Offline(OneSVServer &server) {
	BackupUsedAgain = 0;
	memset(Parity, 0, sizeof(uint64_t) * B * M * 2);
	// For offline generation the indicator bit is always set to 1.
	memset(IndicatorBit, 255, (M+7)/8);
	
	uint32_t InvalidHints = 0;
	uint32_t prfOut [4]; 
	for (uint32_t hint_i = 0; hint_i < M + M/2; hint_i++) {
		// Find the cutoffs for each hint
		if ((hint_i % 4) == 0) {
			for (uint32_t k = 0; k < PartNum; k++) {
				prf.evaluate((uint8_t*) prfOut, hint_i / 4, k, 1);
				for (uint8_t l = 0; l < 4; l++) {
					prfSelectVals[PartNum*l+ k] = prfOut[l];
				}
			}
		}
		SelectCutoff[hint_i] = FindCutoff(prfSelectVals + PartNum*(hint_i%4), PartNum);
		InvalidHints += !SelectCutoff[hint_i];	
	}
	cout << "Offline: cutoffs done, invalid hints: " << InvalidHints << endl;

	uint16_t prfIndices [8];
	for (uint32_t hint_i = 0; hint_i < M; hint_i++) {
		HintID[hint_i] = hint_i;

		uint16_t ePart;
		bool b = 1;
		while (b)	{
			// Keep picking until hitting an un-selected partition
			ePart = NextDummyIdx() % PartNum; 
			b = prf.PRF4Select(hint_i, ePart, SelectCutoff[hint_i]);	
		}
		uint16_t eIdx = NextDummyIdx() % PartSize;
		ExtraPart[hint_i] = ePart;
		ExtraOffset[hint_i] = eIdx;
	}
	cout << "Offline: extra indices done." << endl;

  // Run Algorithm 4
  // Simulates streaming the entire database one partition at a time.
	uint64_t *DBPart = new uint64_t [PartSize * B]; 
	for (uint32_t part_i = 0; part_i < PartNum; part_i++) {
		for (uint32_t j = 0; j < PartSize; j++) {
			server.getEntry(part_i*PartSize + j, DBPart + j * B);
		}
	
		for (uint32_t hint_i = 0; hint_i < M + M/2; hint_i++) {
			// Compute parities for all hints involving the current loaded partition.
			if ((hint_i % 4) == 0) {
				// Each prf evaluation generates the v values for 4 consecutive hints
				prf.evaluate((uint8_t*) prfOut, hint_i / 4, part_i, 1);
			}
			if ((hint_i % 8) == 0) {
				// Each prf evaluation generates the in-partition offsets for 8 consecutive hints
				prf.evaluate((uint8_t*) prfIndices, hint_i / 8, part_i, 2);
			}
			bool b = prfOut[hint_i % 4] < SelectCutoff[hint_i];
			uint16_t r = prfIndices[hint_i % 8] & (PartSize-1);	// faster than mod 
				
			if (hint_i < M) {
				if (b) {
					for (uint32_t l = 0; l < B; l++) {
						Parity[hint_i*B+l] ^= DBPart[r*B+l];
					}
				}
				else if (ExtraPart[hint_i] == part_i) {
					for (uint32_t l = 0; l < B; l++) {
						Parity[hint_i*B+l] ^= DBPart[ExtraOffset[hint_i] * B + l];
					}
				}
			} else {
				// construct backup hints in pairs
				uint32_t dst = hint_i * B + b * B * M/2;
				for (uint32_t l = 0; l < B; l++) {
					Parity[dst+l] ^= DBPart[r*B+l];
				}
			}
		}
	}

	NextHintIndex = M;
}

void OneSVClient::Online(OneSVServer &server, uint32_t query, uint64_t *result)
{
	assert(query <= N);
	uint16_t queryPartNum = query / PartSize;
	uint16_t queryOffset = query & (PartSize-1);
	bool b_indicator = 0;

	// Run Algorithm 2
	// Find a hint that has our desired query index
	// 找到query位于大那个hint中
	uint64_t hintIndex = find_hint(query, queryPartNum, queryOffset, b_indicator);
	assert(hintIndex < M);

	// Build a query. 
	uint32_t hintID = HintID[hintIndex];
	uint32_t cutoff = SelectCutoff[hintIndex];
	// Randomize the selector bit that is sent to the server.
	bool shouldFlip = rand() & 1;

	if (hintID > M){
		BackupUsedAgain++;
	}
	//这个for循环的内容未知，
	for (uint32_t part = 0; part < PartNum; part++){
		if (part == queryPartNum)	{
			// Current partition is the queried partition 
			bvec[part] = !b_indicator ^ shouldFlip;	// Assign to dummy query
			Svec[part] = NextDummyIdx() & (PartSize-1);
			continue; 
		} else if (ExtraPart[hintIndex] == part) {
			// Current partition is the hint's extra partition
			bvec[part] = b_indicator ^ shouldFlip;	// Assign to real query
			Svec[part] = ExtraOffset[hintIndex];
			continue;
		}	

		bool b = prf.PRF4Select(hintID, part, cutoff);
		bvec[part] =  b ^ shouldFlip;
		if (b == b_indicator) {
			// Assign part to real query
			Svec[part] = prf.PRF4Idx(hintID, part) & (PartSize-1); 
		} else {
			// Assign part to dummy query
			Svec[part] = NextDummyIdx() & (PartSize-1);	
		}
	}
	//bvec, Svec是 client 发送给 server 的内容
	//cout<<"Client query size: "<<PartNum * (32+1)/8<<" bytes"<<endl;
	// Make our query

	/*
	for (uint32_t l = 0; l < PartNum; l++) {
		cout<<bvec[l]<<" "; 
	}cout<<endl;
	for (uint32_t l = 0; l < PartNum; l++) {
		cout<<Svec[l]<<" "; 
	}cout<<endl;
	*/
	//大胆猜测，不想跑了，bvec分配两边，
	//然后(!b_indicator ^ shouldFlip) 确认是real还是dummy。
	memset(Response_b0, 0, sizeof(uint64_t) * B);
	memset(Response_b1, 0, sizeof(uint64_t) * B);
	server.onlineQuery(bvec, Svec, Response_b0, Response_b1);
	//cout<<"Server response size: "<<B*64/8<<" bytes"<<endl;
	// Response_b0, Response_b1 是 server 回复的内容

	// Set the query result to the correct response.
	uint64_t * QueryResult = (!b_indicator ^ shouldFlip) ? Response_b0 : Response_b1;
	
	//cout<<"B: "<<B<<endl;
	for (uint32_t l = 0; l < B; l++) { // 这里的B是指有原先32B打包成B个uint64
		result[l] = QueryResult[l] ^ Parity[hintIndex*B + l]; 
	}
	// result 就是回答的内容

#ifdef DEBUG
	// Check for correctness 
	server.getEntry(query, tmpEntry);

	for (uint32_t l = 0; l < B; l++){
		if (result[l] != tmpEntry[l]) {
			cout << "Query failed, debugging query " << query << endl;
			assert(query < N);
			cout << "Computed result: " << result[l] << "\n" << "Actual entry value: " << tmpEntry[l] << endl;
			cout << "Parity sent by server: " << QueryResult[l] << "\nHint parity: " << Parity[hintIndex*B + l] << endl;
			cout << "Extra entry partition num: " << ExtraPart[hintIndex] << "\nExtra entry offset: " << ExtraOffset[hintIndex] << endl;

			assert(0);
		}
	}
#endif

	//这里就是更新内容了。

	while (SelectCutoff[NextHintIndex] == 0) {
		// skip invalid hints
		NextHintIndex++;
	}

  // Run Algorithm 5
  // Replenish a hint using a backup hint.
	HintID[hintIndex] = NextHintIndex;
	SelectCutoff[hintIndex] = SelectCutoff[NextHintIndex];
	ExtraPart[hintIndex] = queryPartNum;
	ExtraOffset[hintIndex] = queryOffset; 
	// Set the indicator bit to exclude queryPartNum  
	b_indicator = !prf.PRF4Select(NextHintIndex, queryPartNum, SelectCutoff[NextHintIndex]);		
	IndicatorBit[hintIndex/8] = (IndicatorBit[hintIndex/8] & ~(1 << (hintIndex%8))) | (b_indicator << (hintIndex % 8));
	uint32_t parity_i = NextHintIndex*B;
	parity_i += ((IndicatorBit[hintIndex/8] >> (hintIndex % 8)) & 1) * B * M/2;
	for (uint32_t l = 0; l < B; l++){
		Parity[hintIndex*B+l] = Parity[parity_i+l] ^ result[l];
	}
	NextHintIndex++;
	assert(NextHintIndex < (M + M/2));
	//如果backup消耗完了，就要重新运行了。
}