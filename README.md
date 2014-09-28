SYNOPSIS
========

sff_splitter is a fast, OpenMP multithreaded, SFF demultiplexer written in C++.  
Given a list of adaptor sequences and a SFF file, sff_splitter extracts read information from the file and split the reads by the adaptor sequence they match. It outputs its results in adaptor-specific SFF files. 

sff_splitter allows for imperfect matching between reads and adaptor sequences. The degree of mismatch is controlled by the user.

sff_splitter does not require that all adaptor sequences have the same length. 

USAGE
=====

Given an SFF file, `file.sff`, and a tab-separated list of adaptors (see below) `adaptors.txt`, you can simply run:

    sff_splitter -i file.sff -a adaptors.txt -o output_stem 

This will produce files named `output_stem.adaptor_name.sff` (one per matched adaptor) and an additional file named `output_stem.unmatched.sff`. 
The unmatched file contains reads that could not be mapped to any adaptor sequence. 

REQUIREMENTS
============
sff_splitter requires gcc version >= 4.7 and the openmp library. 

OPTIONS
=======

Below is the help message (via `sff_splitter -h`) describing its usage & options:

Usage: sff_splitter [arguments]
	Required arguments:
		-i <input.sff>      Input file to split.
		-a <adaptors.txt>   Adaptors used to split input file. Format: <name>	<sequence>.
		-o <output_stem>    Stem for output file. Output will be stored as '<output_stem>.adaptor.sff'
	Optional arguments:
		-h                  This help message
		-v                  verbose
		-m <VALUE>          Maximum number of mismatches between adaptor and read. Default: 0
		-t <VALUE>          Number of threads    Default: 1
		-b <VALUE>          Read buffer size     Default: 100


INSTALLATION
============

The installation process currently consists of a very simple Makefile.

Just do the following:

    git clone https://github.com/edurand/sffsplitter
    cd sffsplitter
    make 


AUTHORS
=======

Eric Y. Durand (durand.ey@gmail.com)

ACKNOWLEDGEMENTS
================

The Big-Endian conversion routine headers have been taken from sff2fastq, developped by Indraniel Das. 
The structure of the SFF reader has also been inspired by sff2fastq. 
https://github.com/indraniel/sff2fastq/



DISCLAIMER
==========
This software is provided "as is" without warranty of any kind.

