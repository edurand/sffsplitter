#include <stdexcept>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include "sff.hpp"

namespace sff
{
    /*** Befin SFFFileHeader implementation ***/
    bool SFFFileHeader::validate() const
    {
        /* Ensure that the magic byte is valid */
        if (magic != SFF_MAGIC) {
            std::cerr << "The SFF header has magic value "
                      << (int)(magic) << std::endl;
            std::cerr << "Program expects SFF magic value to be " 
                      << (int)(SFF_MAGIC) << std::endl;
            return false; 
        }

        /* ensure that the version header is valid */
        if ( memcmp(version, SFF_VERSION, SFF_VERSION_LENGTH) ) {
            std::cerr << "The SFF file has header version: "; 
            int i;
            for (i=0; i < SFF_VERSION_LENGTH; i++) {
                printf("0x%02x ", version[i]);
            }
            printf("\n");
            std::cerr << "Program expects SFF header version: ";
            for (i = 0; i < SFF_VERSION_LENGTH; i++) {
                printf("0x%02x ", SFF_VERSION[i]);
            }
            printf("\n");
            return false;
        }
        
        if (flow.size() != flow_len)
        {
            std::cerr << "Error reading SFF header flow" << std::endl;
            return false;
        }
        if (key.size() != key_len)
        {
            std::cerr << "Error reading SFF header key" << std::endl;
            return false;
        }
        return true;
    }

    int SFFFileHeader::get_size() const
    {
        int header_size = sizeof(magic)
                        + sizeof(char)*4
                        + sizeof(index_offset)
                        + sizeof(index_len)
                        + sizeof(nreads)
                        + sizeof(header_len)
                        + sizeof(key_len)
                        + sizeof(flow_len)
                        + sizeof(flowgram_format)
                        + (sizeof(flow[0]) * flow_len)
                        + (sizeof(key[0]) * key_len);
        return header_size; 
    }

    void SFFFileHeader::host_to_big_endian()
    {
        magic        = htobe32(magic);
        index_offset = htobe64(index_offset);
        index_len    = htobe32(index_len);
        nreads       = htobe32(nreads);
        header_len   = htobe16(header_len);
        key_len      = htobe16(key_len);
        flow_len     = htobe16(flow_len);
    }

    void SFFFileHeader::big_endian_to_host()
    {
        magic        = be32toh(magic);
        index_offset = be64toh(index_offset);
        index_len    = be32toh(index_len);
        nreads       = be32toh(nreads);
        header_len   = be16toh(header_len);
        key_len      = be16toh(key_len);
        flow_len     = be16toh(flow_len);
    }

    /*** Begin SFFReadHeader implementation ***/
    int SFFReadHeader::get_size() const
    {
        int header_size = sizeof(header_len)
                          + sizeof(name_len)
                          + sizeof(nbases)
                          + sizeof(clip_qual_left)
                          + sizeof(clip_qual_right)
                          + sizeof(clip_adapter_left)
                          + sizeof(clip_adapter_right)
                          + (sizeof(char) * name_len);
        return header_size;
    }

    void SFFReadHeader::host_to_big_endian()
    {
        header_len         = htobe16(header_len);
        name_len           = htobe16(name_len);
        nbases             = htobe32(nbases);
        clip_qual_left     = htobe16(clip_qual_left);
        clip_qual_right    = htobe16(clip_qual_right);
        clip_adapter_left  = htobe16(clip_adapter_left);
        clip_adapter_right = htobe16(clip_adapter_right);
    }

    void SFFReadHeader::big_endian_to_host()
    {
        header_len         = be16toh(header_len);
        name_len           = be16toh(name_len);
        nbases             = be32toh(nbases);
        clip_qual_left     = be16toh(clip_qual_left);
        clip_qual_right    = be16toh(clip_qual_right);
        clip_adapter_left  = be16toh(clip_adapter_left);
        clip_adapter_right = be16toh(clip_adapter_right);
    }

    

    /*** Begin SFFReadData implementation ***/
    SFFReadData::SFFReadData(uint16_t flow_len, uint32_t nbases) :
        flowgram(flow_len), 
        flow_index(nbases), 
        bases(nbases), 
        quality(nbases)
    {}

    int SFFReadData::get_size() const
    {
        int flow_len = flowgram.size();
        int nbases = bases.size(); 
        int data_size = (sizeof(flowgram[0]) * flow_len) 
                        + (sizeof(flow_index[0]) * nbases) 
                        + (sizeof(bases[0]) * nbases)
                        + (sizeof(quality[0]) * nbases); 
        return data_size; 
    }

    void SFFReadData::host_to_big_endian()
    {
        int flow_len = flowgram.size(); 
        int i;
        for (i = 0; i < flow_len; i++) {
            flowgram[i] = htobe16(flowgram[i]);
        }
    }

    void SFFReadData::big_endian_to_host()
    {
        int flow_len = flowgram.size(); 
        int i;
        for (i = 0; i < flow_len; i++) {
            flowgram[i] = be16toh(flowgram[i]);
        }
    }

    /*** Begin SFFRead implementation ***/
    SFFField::SFFField(const SFFFileHeader &header) :
        key_len(header.key_len), 
        flow_len(header.flow_len),
        key(header.key)
    {}

    SFFField::~SFFField()
    {
        delete header,
        delete data; 
    }

    bool SFFField::validate() const
    {
        if (flow_len != data->flowgram.size())
        {
            std::cerr << "Error reading field flowgram" << std::endl;
            return false;
        }
        if (header->nbases != data->bases.size())
        {
            std::cerr << "Error reading field bases" << std::endl;
            return false;
        }
        if (!std::equal(data->bases.begin(), 
                       data->bases.begin() + key_len, key.begin()))
        {
            std::cerr << "Field does not start with expected key" << std::endl;
            return false;
        }
        return true;
    }

    const SFFReadHeader* SFFField::get_header()
    {
        return header; 
    }

    const SFFReadData* SFFField::get_data()
    {
        return data; 
    }

    std::string SFFField::get_name() const
    {
        return header->name;
    }

    int SFFField::get_flow_len() const
    {
        return flow_len;
    }

    int SFFField::get_key_len() const
    {
        return key_len;
    }

    std::vector<char> SFFField::get_key() const
    {
        return key;
    }

    void SFFField::set_header(const SFFReadHeader *h)
    {
        header = h; 
    }

    void SFFField::set_data(const SFFReadData *d)
    {
        data = d;
    }

    int SFFField::get_left_clip_value() const
    {
        int left_clip = std::max(
               1, (int)std::max(header->clip_qual_left, 
                                header->clip_adapter_left));

        /* account for the 1-based index value */
        left_clip = left_clip - 1;
        return left_clip;
    }

    int SFFField::get_right_clip_value() const
    {
        int right_clip = (int) std::min(
              (header->clip_qual_right == 0 ? header->nbases : header->clip_qual_right),
              (header->clip_adapter_right == 0 ? header->nbases : header->clip_adapter_right)
        );
        return right_clip;
    }

    std::string SFFField::get_left_adaptor_sequence(int size) const
    {
        int left_clip = get_left_clip_value(); 
        /* Don't run over */
        int left_pos = std::min(left_clip, size+key_len);
        std::string retval(data->bases.begin()+key_len, 
                           data->bases.begin()+left_pos); 
        return retval;
    }

    /*** Begin SFFFileReader implementation ***/
    SFFFileReader::SFFFileReader(const std::string &filename) :
        ifs(filename.c_str(), std::ifstream::binary)
    {
        if (!ifs.is_open())
            throw std::runtime_error("Could not open file for reading");
    }

    SFFFileReader::~SFFFileReader()
    {
        ifs.close();
    }

    bool SFFFileReader::done()
    {
        /* Check that we are at end of file */
        char c = ifs.peek(); 
        return (c == EOF); 
    }

    bool SFFFileReader::good()
    {
        /* Check that the file stream is in good state for reading */
        bool good = (ifs && !ifs.eof()); 
        return good; 
    }

    bool SFFFileReader::read_common_header(SFFFileHeader &header)
    {
        ifs.read(reinterpret_cast<char*>(&header.magic),
                 sizeof(header.magic)); 
        if (!good()) return false; 
        ifs.read(header.version,
                 sizeof(header.version[0])*4); 
        if (!good()) return false; 
        ifs.read(reinterpret_cast<char*>(&header.index_offset),
                 sizeof(header.index_offset));
        if (!good()) return false; 
        ifs.read(reinterpret_cast<char*>(&header.index_len),
                 sizeof(header.index_len));
        if (!good()) return false; 
        ifs.read(reinterpret_cast<char*>(&header.nreads), 
                 sizeof(header.nreads));
        if (!good()) return false; 
        ifs.read(reinterpret_cast<char*>(&header.header_len), 
                 sizeof(header.header_len));
        if (!good()) return false; 
        ifs.read(reinterpret_cast<char*>(&header.key_len), 
                 sizeof(header.key_len));
        if (!good()) return false; 
        ifs.read(reinterpret_cast<char*>(&header.flow_len),
                 sizeof(header.flow_len));
        if (!good()) return false; 
        ifs.read(reinterpret_cast<char*>(&header.flowgram_format), 
                 sizeof(header.flowgram_format));
        if (!good()) return false; 
        
        /* sff files are in big endian notation so adjust appropriately */
        header.host_to_big_endian(); 
        
        /* Now we read the flow and the key */
        header.key = std::vector<char>(header.key_len); 
        header.flow = std::vector<char>(header.flow_len); 
        ifs.read(reinterpret_cast<char*>(&header.flow[0]), 
                 sizeof(header.flow[0])*header.flow_len); 
        if (!good()) return false;
        ifs.read(reinterpret_cast<char*>(&header.key[0]), 
                 sizeof(header.key[0])*header.key_len);
        if (!good()) return false;

        /* the common header section should be a multiple of 8-bytes 
        if the header is not, it is zero-byte padded to make it so */
        int header_size = header.get_size(); 
        bool readPadding = true;
        if ( !(header_size % PADDING_SIZE == 0) ) {
            readPadding = read_padding(header_size);
        }
        return (header.validate() && readPadding);
    }

    bool SFFFileReader::read_field_header(SFFReadHeader *header)
    {
        ifs.read(reinterpret_cast<char*>(&(header->header_len)), 
                 sizeof(header->header_len));
        if (!good()) return false;
        ifs.read(reinterpret_cast<char*>(&(header->name_len)),
                 sizeof(header->name_len));
        if (!good()) return false;
        ifs.read(reinterpret_cast<char*>(&(header->nbases)), 
                 sizeof(header->nbases));
        if (!good()) return false;
        ifs.read(reinterpret_cast<char*>(&(header->clip_qual_left)), 
                 sizeof(header->clip_qual_left));
        if (!good()) return false;
        ifs.read(reinterpret_cast<char*>(&(header->clip_qual_right)), 
                 sizeof(header->clip_qual_right));
        if (!good()) return false;
        ifs.read(reinterpret_cast<char*>(&(header->clip_adapter_left)), 
                 sizeof(header->clip_adapter_left));
        if (!good()) return false;
        ifs.read(reinterpret_cast<char*>(&(header->clip_adapter_right)), 
                 sizeof(header->clip_adapter_right));
        if (!good()) return false;

        /* sff files are in big endian notation so adjust appropriately */
        header->host_to_big_endian(); 

        /* finally appropriately allocate and read the read_name string */
        char name[header->name_len];
        ifs.read(name, sizeof(char)*(header->name_len));
        if (!good()) return false;
        header->name = std::string(name, name+(header->name_len));

        /* the section should be a multiple of 8-bytes, if not,
           it is zero-byte padded to make it so */
        int header_size = header->get_size();  
        bool readPadding = true; 
        if ( !(header_size % PADDING_SIZE == 0) ) {
            readPadding = read_padding(header_size);
        }
        return readPadding; 
    }

    bool SFFFileReader::read_field_data(SFFReadData *data)
    {

        /* read from the sff file and assign to vectors*/
        ifs.read(reinterpret_cast<char*>(&(data->flowgram[0])),
                 sizeof(data->flowgram[0])*data->flowgram.size());
        if (!good()) return false;
        ifs.read(reinterpret_cast<char*>(&(data->flow_index[0])), 
                 sizeof(data->flow_index[0])*data->flow_index.size());
        if (!good()) return false;
        ifs.read(reinterpret_cast<char*>(&(data->bases[0])),
                 sizeof(data->bases[0])*data->bases.size());
        if (!good()) return false;
        ifs.read(reinterpret_cast<char*>(&(data->quality[0])),    
                 sizeof(data->quality[0])*data->quality.size());
        if (!good()) return false;

        /* sff files are in big endian notation so adjust appropriately */
        data->host_to_big_endian(); 

        /* the section should be a multiple of 8-bytes, if not,
           it is zero-byte padded to make it so */
        int data_size = data->get_size();  
        bool readPadding = true;              
        if ( !(data_size % PADDING_SIZE == 0) ) {
            readPadding = read_padding(data_size);
        }
        return readPadding;
    }

    bool SFFFileReader::read_field(SFFField &field)
    {
        SFFReadHeader *header = new SFFReadHeader(); 
        bool headerRead = read_field_header(header);
        if (!headerRead)
            return false; 
        SFFReadData *data = new SFFReadData(field.get_flow_len(), header->nbases); 
        bool dataRead = read_field_data(data); 
        if (!dataRead)
            return false;
        field.set_header(header); 
        field.set_data(data);
        return field.validate();
   }

    bool SFFFileReader::read_padding(int size)
    {
        int remainder = PADDING_SIZE - (size % PADDING_SIZE);
        char padding[remainder];
        ifs.read(padding, sizeof(uint8_t)*remainder); 
        return true;
    }
    
    /* Begin SFFFileWriter implementation 
     * We open of outfile stream in mode binary | in so we can write 
     * binary data and update the common header in place after we have
     * figured out how many reads match an adaptor. We keep a counter
     * of reads we successfully write. 
     */
    SFFFileWriter::SFFFileWriter(const std::string &filename) : 
        ofs(filename.c_str(), std::ios::out |
                              std::ios::trunc |
                              std::ios::binary | 
                              std::ios::in),
        nreads(0)
    {
        if (!ofs.is_open())
            throw std::runtime_error("Could not open file for writing");
    }

    SFFFileWriter::~SFFFileWriter()
    {
        ofs.close();
    }
    
    int SFFFileWriter::get_number_of_fields_written()
    {
        return nreads;
    }

    bool SFFFileWriter::write_common_header(const SFFFileHeader &header)
    {
        /* nreads will need to be updated after we know the right number of reads
         * matching adaptor. The rest of the common header does not need to change
         * but we still re-write the full common header for cleaner code
         */
        ofs.seekp(std::ofstream::beg); 
        SFFFileHeader h(header); 
        h.big_endian_to_host(); 
        ofs.write(reinterpret_cast<char*>(&h.magic),
                  sizeof(h.magic)); 
        ofs.write(reinterpret_cast<char*>(&h.version[0]),
                  sizeof(char)*4); 
        ofs.write(reinterpret_cast<char*>(&h.index_offset),
                  sizeof(h.index_offset));
        ofs.write(reinterpret_cast<char*>(&h.index_len), 
                  sizeof(h.index_len));
        ofs.write(reinterpret_cast<char*>(&h.nreads),  
                  sizeof(h.nreads));
        ofs.write(reinterpret_cast<char*>(&h.header_len), 
                  sizeof(h.header_len));
        ofs.write(reinterpret_cast<char*>(&h.key_len),    
                  sizeof(h.key_len));
        ofs.write(reinterpret_cast<char*>(&h.flow_len), 
                  sizeof(h.flow_len));
        ofs.write(reinterpret_cast<char*>(&h.flowgram_format), 
                  sizeof(h.flowgram_format));
        ofs.write(reinterpret_cast<char*>(&h.flow[0]),         
                  sizeof(char)*header.flow_len);
        ofs.write(reinterpret_cast<char*>(&h.key[0]),  
                  sizeof(char)*header.key_len);

        int header_size = header.get_size();  
        if ( !(header_size % PADDING_SIZE == 0) ) {
            write_padding(header_size);
        }
        return true;
    }

    bool SFFFileWriter::write_field(SFFField &read)
    {
        bool writeHeader = write_field_header(read.get_header());
        bool writeData = write_field_data(read.get_data()); 
        return (writeHeader && writeData); 
    }

    bool SFFFileWriter::write_field_header(const SFFReadHeader *header)
    {
        /* Make a copy of header to be written as we need to flip endian */
        SFFReadHeader h(*header);
        h.big_endian_to_host(); 
        ofs.write(reinterpret_cast<char*>(&h.header_len), 
                  sizeof(h.header_len));
        ofs.write(reinterpret_cast<char*>(&h.name_len), 
                  sizeof(h.name_len));
        ofs.write(reinterpret_cast<char*>(&h.nbases), 
                  sizeof(h.nbases));
        ofs.write(reinterpret_cast<char*>(&h.clip_qual_left), 
                  sizeof(h.clip_qual_left));
        ofs.write(reinterpret_cast<char*>(&h.clip_qual_right), 
                  sizeof(h.clip_qual_right));
        ofs.write(reinterpret_cast<char*>(&h.clip_adapter_left), 
                  sizeof(h.clip_adapter_left));
        ofs.write(reinterpret_cast<char*>(&h.clip_adapter_right), 
                  sizeof(h.clip_adapter_right));
        ofs.write(h.name.c_str(), h.name.size());

        /* the section should be a multiple of 8-bytes, if not,
           it is zero-byte padded to make it so */
        int header_size = header->get_size();  
        bool writePadding = true; 
        if ( !(header_size % PADDING_SIZE == 0) ) {
            writePadding = write_padding(header_size);
        }
        if (writePadding)
            nreads++; 
        return writePadding; 
    }

    bool SFFFileWriter::write_field_data(const SFFReadData *data)
    {
        /* Make a copy of data to be written as we need to flip endian */
        SFFReadData d(*data); 
        d.big_endian_to_host(); 

        int data_size = data->get_size();  
        int flow_len = data->flowgram.size(); 
        int nbases = data->bases.size(); 
        ofs.write(reinterpret_cast<char*>(&d.flowgram[0]),   
                  sizeof(d.flowgram[0])*flow_len);
        ofs.write(reinterpret_cast<char*>(&d.flow_index[0]), 
                  sizeof(d.flow_index[0])*nbases);
        ofs.write(reinterpret_cast<char*>(&d.bases[0]),      
                  sizeof(d.bases[0])*nbases);
        ofs.write(reinterpret_cast<char*>(&d.quality[0]),    
                  sizeof(d.quality[0])*nbases);

        /* the section should be a multiple of 8-bytes, if not,
           it is padded to make it so */
        bool writePadding = true;              
        if ( !(data_size % PADDING_SIZE == 0) ) {
            writePadding = write_padding(data_size);
        }
        return writePadding;
    }

    bool SFFFileWriter::write_padding(int size)
    {
        int remainder = PADDING_SIZE - (size % PADDING_SIZE);
        char padding[remainder];
        ofs.write(padding, sizeof(uint8_t)*remainder); 
        return true;
    }
}

