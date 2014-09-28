#include <omp.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <getopt.h>
#include <stdio.h>
#include "sff.hpp"
#include "adaptors.hpp"

#define PRG_NAME "sff_splitter"

/* Required arguments */
std::string infilename; 
std::string outstem; 
std::string adaptorfilename;

/* Options */
int maxmismatch=0;
bool verbose=false;
int num_threads=1;
int buffer_size=100; 

/* Map from adaptor name to writer */
typedef std::unordered_map<std::string, sff::SFFFileWriter*> outmap; 
typedef std::vector<sff::SFFField*> fieldbuffer; 
typedef std::vector<sff::SFFField*>::iterator bufferiter; 

void print_help_message()
{
    printf("Usage: %s %s\n", PRG_NAME, "[arguments]");
    printf("\tRequired arguments:\n");
    printf("\t\t%-20s%-20s\n", "-i <input.sff>", "Input file to split.");
    printf("\t\t%-20s%-20s %s\n", 
                    "-a <adaptors.txt>", 
                    "Adaptors used to split input file.",
                    "Format: <name>\t<sequence>.");
    printf("\t\t%-20s%-20s %s\n",
                    "-o <output_stem>",
                    "Stem for output file.",
                    "Output will be stored as '<output_stem>.adaptor.sff'");

    printf("\tOptional arguments:\n");
    printf("\t\t%-20s%-20s\n", "-h", "This help message");
    printf("\t\t%-20s%-20s\n", "-v", "verbose");
    printf("\t\t%-20s%-20s %s %d\n", 
                    "-m <VALUE>", 
                    "Maximum number of mismatches between adaptor and read.",
                    "Default:",
                    maxmismatch);
    printf("\t\t%-20s%-20s %s %d\n", 
                    "-t <VALUE>", 
                    "Number of threads",
                    "Default:",
                    num_threads);
    printf("\t\t%-20s%-20s %s %d\n", 
                    "-b <VALUE>", 
                    "Read buffer size",
                    "Default:",
                    buffer_size);
}

void parse_arguments(int argc, char** argv)
{
    int c;
    while ((c = getopt(argc, argv, "hvi:a:o:pm:t:b:")) != EOF) {
        switch(c) {
            case 'h':
                print_help_message(); 
                exit(0);
                break;
            case 'v':
                verbose=true;
                break;
            case 'i':
                infilename = std::string(optarg);
                break;
            case 'a': 
                adaptorfilename = std::string(optarg);
                break;
            case 'o':
                outstem = std::string(optarg); 
                break;
            case 'm':
                maxmismatch=atoi(optarg);
                if (maxmismatch < 0)
                {
                    std::cerr << "Maximum number of mismatch "
                              << "must be 0 or greater" << std::endl;
                    exit(1); 
                }
                break;
            case 't':
                num_threads = atoi(optarg); 
                if (num_threads < 1)
                {
                    std::cerr << "Number of threads must be at least 1" << std::endl;
                    exit(1);
                }
                break;
            case 'b':
                buffer_size = atoi(optarg); 
                if (buffer_size < 1)
                {
                    std::cerr << "Size of read buffer must be at least 1" << std::endl;
                    exit(1); 
                }
                break;
            case '?':
                print_help_message(); 
                exit(1); 
                break;
            default: 
                abort(); 
        }
    }
    if (infilename.size() == 0) 
    {
        std::cerr << "input filename is required" << std::endl;
        print_help_message(); 
        exit(1); 
    }
    if (outstem.size() == 0)
    {
        std::cerr << "output stem is required" << std::endl;
        print_help_message(); 
        exit(1); 
    }
    if (adaptorfilename.size() == 0)
    {
        std::cerr << "adaptor filename is required" << std::endl;
        print_help_message(); 
        exit(1);
    }
}

std::string get_adaptor_outfile(const std::string adaptor_name)
{
    std::string retval(outstem); 
    retval.append(".");
    retval.append(adaptor_name);
    retval.append(".sff");
    return retval;
}

void empty_buffer(fieldbuffer &buffer)
{
    bufferiter iter;
    int size = buffer.size(); 
    for (iter = buffer.begin(); iter != buffer.end(); ++iter)
    {
        delete *iter;
    }
    buffer.clear(); 
    buffer.resize(size);
}

int main(int argc, char** argv)
{
    parse_arguments(argc, argv); 
    
    omp_set_num_threads(num_threads);

    sff::AdaptorFinder adaptorFinder(maxmismatch); 
    adaptorFinder.read(adaptorfilename); 

    outmap outputMap; 
    outmap::const_iterator outputIterator; 

    sff::SFFFileReader reader(infilename); 
    sff::SFFFileHeader common_header; 


    bool hasRead = reader.read_common_header(common_header); 
    if (!hasRead)
    {
        std::cerr << "Failed to read common header" << std::endl;
        exit(2); 
    }
    int nreads = common_header.nreads;
    if (verbose)
    {
        printf("Common header summary:\n");
        printf("\t%-30s%-20d\n", "flow_len : ", common_header.flow_len);
        printf("\t%-30s%-20d\n", "key_len: ", common_header.key_len); 
        printf("\t%-30s%-20d\n", "Number of reads: ", nreads);
    }

    int cpt = 0;         // Count number of reads
    int notfound = 0;    // Count number of reads that were not found

    /* Buffer for multi-threading */
    fieldbuffer buffer(buffer_size); 
    
    while (true)
    {
        /* Filling buffer */
        int buffer_len = 0;     
        while (buffer_len < buffer_size && !reader.done())
        {
            sff::SFFField *field = new sff::SFFField(common_header); 
            if (!reader.read_field(*field))
            {
                std::cerr << "Error reading field" << std::endl;
                exit(2);
            }
            buffer[buffer_len] = field;
            buffer_len ++;
        }
        if (buffer_len == 0) break;

        if (cpt >= nreads)
        {
            std::cerr << "Too many reads in SFF file." << std::endl;
            exit(2); 
        }
        /* Now process buffer */
        #pragma omp parallel for schedule(static,1) ordered
        for (int b = 0; b < buffer_len; b++)
        {
            /* Attempt to find a matching adaptor in current read */
            std::string match; 
            bool found = adaptorFinder.find(*buffer[b], match); 
            if (!found)
            {
                match = UNMATCHED; // Set match name to unmatched string
                notfound ++; 
            }
            #pragma omp ordered
            {
                outputIterator = outputMap.find(match); 
                if (outputIterator == outputMap.end())
                {
                    /* This is the first time we find this adaptor */
                    std::string adaptorfilename = get_adaptor_outfile(match); 
                    sff::SFFFileWriter *writer = new sff::SFFFileWriter(adaptorfilename); 
                    outputMap[match] = writer; 
                    outputMap[match]->write_common_header(common_header); 
                }
                if (!outputMap[match]->write_field(*buffer[b]))
                {
                    std::cerr << "Could not write field to disk" << std::endl;
                    exit(2); 
                }
            }
        }
        cpt += buffer_len; 
        /* clean-up buffer */
        empty_buffer(buffer); 
    }
    if (cpt < nreads)
    {
        std::cerr << "Incorrect number of reads from SFFFile: " << std::endl;
        std::cerr << "\tExpected " << nreads << std::endl
                  << "\tRead " << cpt << std::endl;
        exit(2); 
    }

    if (verbose)
    {
        /* Print summary of run */
        printf("Run summary:\n");
        printf("\t%-30s%-20d\n", "Successfully read: ", cpt);
        printf("\t%-30s%-20d\n", "Unmatched: ", notfound); 
        printf("\t%-30s%-20d\n", "Matched to an adaptor: ", cpt-notfound);
    }
    /* Finally, we need to update the common headers of 
     * adaptor specific files
     */
    for (outputIterator = outputMap.begin(); 
         outputIterator != outputMap.end(); 
         ++outputIterator)
    {
        int nreads = outputIterator->second->get_number_of_fields_written(); 
        sff::SFFFileHeader fixedHeader(common_header); 
        fixedHeader.nreads = nreads;
        outputIterator->second->write_common_header(fixedHeader); 
        if (verbose)
            printf("\t\t%-30s%-20d\n", outputIterator->first.c_str(), nreads);
        delete outputIterator->second;
    }

    return 0;
}
