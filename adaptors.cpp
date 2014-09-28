#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include "adaptors.hpp"
#include "sff.hpp"

namespace sff
{
    /* Being AdaptorAligner implememtation */    
    AdaptorAligner::AdaptorAligner(const std::string &s1, 
                                 const std::string &s2) :
        match_score(0), 
        gap_score(1), 
        mismatch_score(1),
        p1(&s1), 
        p2(&s2)
    {
        int l1 = p1->size(); 
        int l2 = p2->size(); 
        /* Initialization of matrix */
        scoremat.reserve(l1+1);
        int i;
        for (i = 0; i < l1+1; i++)
        {
            std::vector<int> tmp(l2+1, 0); 
            scoremat.push_back(tmp); 
        }
        for (i = 0; i < l1+1; i++)
            scoremat[i][0] = i*gap_score; 
        for (i = 0; i < l2+1; i++)
            scoremat[0][i] = i*gap_score;
    }

    AdaptorAligner::~AdaptorAligner()
    {}

    int AdaptorAligner::compute_alignment_score()
    {
        int l1 = p1->size(); 
        int l2 = p2->size(); 
        int score_up, score_left, score_diag, score_match, curr_score; 
        std::string::const_iterator rowiterator, coliterator;  
        int row = 1; 
        int col;
        for (rowiterator = p1->begin(); rowiterator != p1->end(); ++rowiterator) 
        {
            col = 1;
            for (coliterator = p2->begin(); coliterator != p2->end(); ++coliterator)
            {
                bool equal = (*rowiterator == *coliterator); 
                score_match = equal ? match_score : mismatch_score; 
                score_up   = scoremat[row-1][col] + gap_score; 
                score_left = scoremat[row][col-1] + gap_score;
                score_diag = scoremat[row-1][col-1] + score_match; 
                curr_score = std::min(
                    score_up, std::min(score_left, score_diag)
                );

                scoremat[row][col] = curr_score;  
                col ++; 
            }
            row ++; 
        }
        return scoremat[l1][l2];
    }

    /* Begin AdaptorFinder implementation */
    AdaptorFinder::AdaptorFinder(int maxmismatch) :
        maxmismatch(maxmismatch)
    {
        if (maxmismatch < 0)
            throw std::runtime_error("maxmismatch must be greater or equal than 0");
    }

    AdaptorFinder::~AdaptorFinder()
    {}

    bool AdaptorFinder::read(const std::string &filename)
    {
        std::ifstream ifs(filename.c_str()); 
        if (!ifs.is_open())
            throw std::runtime_error("Could not open adaptor file for reading");
        std::string name, sequence;
        while (ifs >> name >> sequence)
        {
            /* adaptors is a hash table allowing fast adaptor sequence lookup
             * length_of_adaptor -> adaptor_sequence -> adaptor_name
             */
            adaptors[sequence.size()].insert(std::make_pair(sequence, name)); 
        }
        ifs.close();
        return true;
    }

    bool AdaptorFinder::find(const SFFField &field,
                             std::string &match)
    {
        adaptormap::const_iterator sizeiter; 
        stringmap::const_iterator striter;
        std::string sequence; 
        for (sizeiter = adaptors.begin(); sizeiter != adaptors.end(); ++sizeiter)
        {
            sequence = field.get_left_adaptor_sequence(sizeiter->first); 
            /* First check if adaptor runs over to actual read.
             * If yes, then we consider the adaptor does not match.
             */
            if (sequence.size() < sizeiter->first)
                continue;
            striter = sizeiter->second.find(sequence); 
            if (striter != sizeiter->second.end())
            {
                /* We found a perfect match */
                match = striter->second; 
                return true; 
            }
        }
        /* We have not found a perfect match. 
         * Attempt to find an imperfect one.
         */
        if (maxmismatch > 0)
            return find_imperfect(field, match); 
        return false;
    }

    /* Look for imperfect match using Levenstein distance */
    bool AdaptorFinder::find_imperfect(const SFFField &field, 
                                       std::string &match)
    {
        adaptormap::const_iterator sizeiter; 
        stringmap::const_iterator striter;
        int best = maxmismatch+1;
        int alignment = 0;
        std::string sequence;
        for (sizeiter = adaptors.begin(); 
             sizeiter != adaptors.end(); 
             ++sizeiter)
        {
            sequence = field.get_left_adaptor_sequence(sizeiter->first); 
            for (striter = sizeiter->second.begin();
                 striter != sizeiter->second.end(); 
                 ++striter)
            {
                AdaptorAligner sw(sequence, striter->first); 
                alignment = sw.compute_alignment_score();
                if (alignment < best)
                {
                    best = alignment;
                    match = striter->second;
                }
            }
        }
        return (best <= maxmismatch); 
    }
}
