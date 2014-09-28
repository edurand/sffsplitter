#ifndef _SFFSPLITTER_ADAPTORS_HPP_
#define _SFFSPLITTER_ADAPTORS_HPP_

#include <unordered_map>
#include <string>
#include <vector>
#include "sff.hpp"

#define UNMATCHED "unmatched"

namespace sff
{
    
    typedef std::vector<std::vector<int> > intmatrix; 
    class AdaptorAligner
    {
        public:
            AdaptorAligner(const std::string &s1, 
                          const std::string &s2); 
            ~AdaptorAligner(); 

            int compute_alignment_score(); 

        private: 
            int match_score; 
            int gap_score;
            int mismatch_score;
            intmatrix scoremat; 
            const std::string *p1;
            const std::string *p2; 
    };

    /* constant time access string->string dictionary. Stores
     * adapter sequence -> adapter name
     */
    typedef std::unordered_map<std::string,std::string> stringmap;
    /* constant time access int->stringmap. Used to stored adaptors
     * by length
     */
    typedef std::unordered_map<int, stringmap> adaptormap; 

    /* Class to search for a match between a provided adaptor sequence 
     * and the list of adaptors we wish to split on. 
     */
    class AdaptorFinder
    {
        public:
            AdaptorFinder(int maxmismatch); 
            ~AdaptorFinder(); 

            /* Read a list of adaptors from tab-separated file */
            bool read(const std::string &filename);  

            /* Look if field matches one of the adaptors 
             * If a match is found, then the adaptor name is stored 
             * in match. Else, match is set to UNMATCHED and we return false*/
            bool find(const SFFField &field, 
                      std::string &match); 

        private: 
            int maxmismatch;
            adaptormap adaptors;
            bool find_imperfect(const SFFField &field, 
                                std::string &match);
    };
}
#endif
