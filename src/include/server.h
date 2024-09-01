#pragma once
#include "cryptopp/cryptlib.h"
#include "cryptopp/hex.h"
#include "cryptopp/rijndael.h"
#include "cryptopp/modes.h"
#include "cryptopp/files.h"
#include "cryptopp/osrng.h"
#include "utils.h"

using namespace std;
using namespace CryptoPP;

class Server{};

// Server class for the one server variant
class OneSVServer : Server {
  public:
  OneSVServer(uint64_t * DB_ptr, uint32_t LogN, uint32_t EntryB);
  void getEntry(uint32_t index, uint64_t *result);
  /* Generate a single query using the online server. */
  void onlineQuery(bool* bvec, uint32_t *Svec, uint64_t *b0, uint64_t *b1);

  private:
  uint64_t * DB; // Pointer to database array
  uint32_t N; // Number of database entries
  uint32_t B; // Size of one entry is B * 8 bytes
  uint32_t EntrySize; // Size of an entry in bytes

	uint32_t PartNum; // Number of partitions = sqrt(N)
	uint32_t PartSize; // Number of entries in one partition
	uint32_t lambda; // Correctness parameter
	uint32_t M; // Number of hints
  uint64_t * tmpEntry;
};

// Server class for the two server variant
class TwoSVServer : Server{
  public:
  TwoSVServer(uint64_t * DB_ptr, uint32_t LogN, uint32_t EntryB);
  void getEntryFromServer(uint32_t index, uint64_t *result);
  /* Runs the offline phase, generating hints from hintID 0 to M. Does not allocate memory. 
  */
  void generateOfflineHints(uint32_t M, uint64_t * Parity, uint16_t * ExtraPart, uint16_t * ExtraOffset, uint32_t * SelectCutoff);
  /* Generate parities for a hintID using the offline server. Both parities for b = 0 and b = 1 are returned continguously in the result pointer, with b=0 being the first parity.*/
  void replenishHint(uint64_t hintID, uint64_t * result, uint32_t * SelectCutoff);
  /* Generate a single query using the online server. */
  void onlineQuery(bool * bvec, uint32_t *Svec, uint64_t *b0, uint64_t *b1);

  private:
	uint16_t NextDummyIdx();
	uint16_t prfDummyIndices [8];
	uint64_t dummyIdxUsed;

  uint64_t * DB; // Pointer to database array
  uint32_t N; // Number of database entries
  uint32_t B; // Size of one entry is B * 8 bytes
  uint32_t EntrySize; // Size of an entry in bytes

	uint32_t PartNum; // Number of partitions = sqrt(N)
	uint32_t PartSize; // Number of entries in one partition
	uint32_t lambda; // Correctness parameter
	uint32_t M; // Number of hints
  uint64_t * tmpEntry; // Preallocated space for operations involving a database entry

	PRFPartitionID prf;
};