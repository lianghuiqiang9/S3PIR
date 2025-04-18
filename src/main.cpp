#include <fstream>
#include <chrono>
#include <vector>
#include <type_traits>
#include <unistd.h>

#include "client.h"
#include "server.h"
#include "utils.h"

using namespace std;

uint64_t *DB;

struct Options {
	uint64_t Log2DBSize;
	uint64_t EntrySize;
	string OutputFile;
	bool OneSV;
};

void print_usage(){
	cout << "Usage:	" << endl
				<< "\t./s3pir --one-server <Log2 DB Size> <Entry Size> <Output File>" << endl
				<< "\t./s3pir --two-server <Log2 DB Size> <Entry Size> <Output File>" << endl
				<< "Runs the s3pir protocol on a database with <Log2 DB Size> number of entries and entries of <Entry Size> bytes with either the one server or two server variant. If <Output File> doesn't exist, creates <Output File> and adds profiling data to the file in csv format. Otherwise append it to the end of the file.  " << endl << endl;
}

Options parse_options (int argc, char * argv[])
{
	Options options{false, false};

	try{
		if (argc == 5){
			if (strcmp(argv[1], "--two-server") == 0){
				options.OneSV = 0;
			} else if (strcmp(argv[1], "--one-server") == 0){
				options.OneSV = 1;
			} else {
				print_usage();
				exit(0);
			}
			options.Log2DBSize = stoi(argv[2]);
			options.EntrySize = stoi(argv[3]);
			options.OutputFile = argv[4];
			return options;
		} 
		print_usage();
		exit(0);
	} catch (...) {
		print_usage();
		exit(0);
	}
}

// Helper function since clients have different function signatures between variants
inline void test_client_query(OneSVClient& client, OneSVServer& server, uint32_t & query, uint64_t * result){
	client.Online(server, query, result);
}
inline void test_client_query(TwoSVClient& client, TwoSVServer& server, uint32_t & query, uint64_t * result){
	client.Online(server, server, query, result);
}

template<typename Client, typename Server>
void test_pir(uint64_t kLogDBSize, uint64_t kEntrySize, ofstream &output_csv) 
{
	if (is_same<Client, TwoSVClient>::value && is_same<Server, TwoSVServer>::value) {
		cout << "== Two server variant ==" << endl; 
		output_csv << "Two server, ";
	}  else if (is_same<Client, OneSVClient>::value && is_same<Server, OneSVServer>::value) {
		cout << "== One server variant ==" << endl; 
		output_csv << "One server, ";
	}
	cout << "LogDBSize: " << kLogDBSize << "\nEntrySize: " << kEntrySize << " bytes" << endl;
	output_csv << kLogDBSize << ", " << kEntrySize << ", ";

	// 初始化数据库，生成DB是uint64的足够多维的数组
	initDatabase(&DB, kLogDBSize, kEntrySize); 

	/*
	for (uint32_t l = 0; l < (((uint64_t) kEntrySize / 8) << kLogDBSize); l++) {
		cout<<DB[l]<<" "; 
	}cout<<"  DB"<<endl;
	*/

	Client client(kLogDBSize, kEntrySize);
  	Server server(DB, kLogDBSize, kEntrySize);

	// Client 进行offline操作，生成hint，
	cout << "Running offline phase.." << endl;
	auto start = chrono::high_resolution_clock::now();	
	client.Offline(server);
	auto end = chrono::high_resolution_clock::now();	
	auto offline_time = chrono::duration_cast<chrono::milliseconds>(end - start);
	cout << "Offline: " << (double) offline_time.count() / 1000.0 << " s"<< endl;

	//Client 进行query
	start = chrono::high_resolution_clock::now();	
	uint64_t *result = new uint64_t [kEntrySize/8];
	int num_queries = 1 << (kLogDBSize / 2 + kLogDBSize % 2); 	// Run PartitionSize queries, < half of backup hints
	cout << "Running " << num_queries << " queries" << endl;
	int progress = 0;
	int milestones = num_queries/5;
	for (uint64_t i = 0; i < num_queries; i++)
	{
		if (i % milestones == 0){
			cout << "Completed: " << progress * 20 << "%" << endl;
			progress++;
		} 
		
		uint16_t part = i % (1 << kLogDBSize / 2);
		uint16_t offset = i % (1 << kLogDBSize / 2);
		uint32_t query = (part << (kLogDBSize / 2)) + offset;

		//cout<<"query: "<<query<<endl;
		//这里进行client query; server answer; 和更新hint的操作；
		test_client_query(client, server, query, result);
		
		//验证是否相等
		/*
		uint32_t B = kEntrySize/8; 
		for (uint32_t l = 0; l < B; l++) {
			cout<<DB[query*B+l]<<" "; 
		}cout<<endl;

		for (uint32_t l = 0; l < B; l++) {
			cout<<result[l]<<" "; 
		}cout<<endl;
		*/
		
	}
	end = chrono::high_resolution_clock::now();	
	auto total_online_time = chrono::duration_cast<chrono::milliseconds>( (end - start) );
	double online_time = ((double) total_online_time.count()) / num_queries;

	uint32_t B = kEntrySize/8; 
	uint32_t PartNum = 1 << (kLogDBSize/2);
	cout<<"Client query size: "<<PartNum * (32+1)/8<<" bytes"<<endl;
	cout<<"Server response size: "<<B*64/8<<" bytes"<<endl;

	cout << "Ran " << num_queries << " queries" << endl;
	cout << "Online: " << total_online_time.count() << " ms"<< endl;
	cout << "Cost Per Query: " << online_time << " ms" << endl;

	output_csv << num_queries << ", " << (double) offline_time.count() / 1000.0 << ", " <<  online_time;
	if (is_same<Client, TwoSVClient>::value && is_same<Server, TwoSVServer>::value) {
		output_csv << ", -" ;
	}  else if (is_same<Client, OneSVClient>::value && is_same<Server, OneSVServer>::value) {
		double amortized_compute_time_per_query = ((double) offline_time.count()) / (0.5 * LAMBDA * (1 << (kLogDBSize / 2))) + online_time;
		cout << "Amortized compute time per query: " << amortized_compute_time_per_query  << " ms" << endl; 
		output_csv << ", " << amortized_compute_time_per_query;
	}
	output_csv << endl;
	cout << endl;

	//Server answer


}

int main(int argc, char *argv[]){

	Options options = parse_options(argc, argv);

	ofstream output_csv;
	// If output file doesn't exist then add the headers
	if (access(options.OutputFile.c_str(), F_OK) == -1){
		output_csv.open(options.OutputFile);
		output_csv << "Variant, Log2 DBSize, EntrySize(Bytes), NumQueries, Offline Time (s), Online Time (ms),  Amortized Compute Time Per Query (ms)" <<endl;
	} else {
		output_csv.open(options.OutputFile, ofstream::out | ofstream::app);
	}

	if (options.OneSV){
		test_pir<OneSVClient, OneSVServer> (options.Log2DBSize, options.EntrySize, output_csv);
	} else {
		test_pir<TwoSVClient, TwoSVServer> (options.Log2DBSize, options.EntrySize, output_csv);
	}
	output_csv.close();
}