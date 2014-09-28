#ifndef _SFFSPLITTER_SFF_HPP_
#define _SFFSPLITTER_SFF_HPP_

#include <string>
#include <vector>
#include <fstream>

#if defined __linux__
  #include <endian.h>
#elif defined __BSD__
  #include <sys/endian.h>
#elif defined __APPLE__
  #include <sys/_endian.h>
  #include <architecture/byte_order.h>
  #define __bswap_64(x)      NXSwapLongLong(x)
  #define __bswap_32(x)      NXSwapLong(x)
  #define __bswap_16(x)      NXSwapShort(x)

  #if __DARWIN_BYTE_ORDER == __DARWIN_BIG_ENDIAN
    #define htobe16(x) (x)
    #define htole16(x) __bswap_16 (x)
    #define be16toh(x) (x)
    #define le16toh(x) __bswap_16 (x)

    #define htobe32(x) (x)
    #define htole32(x) __bswap_32 (x)
    #define be32toh(x) (x)
    #define le32toh(x) __bswap_32 (x)

    #define htobe64(x) (x)
    #define htole64(x) __bswap_64 (x)
    #define be64toh(x) (x)
    #define le64toh(x) __bswap_64 (x)
  #else
    #define htobe16(x) __bswap_16 (x)
    #define htole16(x) (x)
    #define be16toh(x) __bswap_16 (x)
    #define le16toh(x) (x)

    #define htobe32(x) __bswap_32 (x)
    #define htole32(x) (x)
    #define be32toh(x) __bswap_32 (x)
    #define le32toh(x) (x)

    #define htobe64(x) __bswap_64 (x)
    #define htole64(x) (x)
    #define be64toh(x) __bswap_64 (x)
    #define le64toh(x) (x)
  #endif
#else
  #error Unknown location for endian.h
#endif

#define SFF_MAGIC   0x2e736666 /* ".sff" */
#define SFF_VERSION "\0\0\0\1"
#define SFF_VERSION_LENGTH 4
#define PADDING_SIZE 8

namespace sff
{
    /* Virtual class representing a field in the SFF field.
     * Mostly, this class is here to enforce implementation of
     * key methods 
     */
    class VirtualSFFField
    {
        public: 
           /* Get size of all struct members. This is different 
             * from sizeof(SFFReadHeader) because C will add padding
             * to optimize memory
             */ 
            virtual int get_size() const = 0;
            /* in-place handle big-endian conversions */
            virtual void host_to_big_endian() = 0;
            virtual void big_endian_to_host() = 0;
    };

    /* Container representing the SFF file common header */
    class SFFFileHeader : public VirtualSFFField
    {
        public: 
            uint32_t magic;
            char version[4];
            uint64_t index_offset;
            uint32_t index_len;
            uint32_t nreads;
            uint16_t header_len;
            uint16_t key_len;
            uint16_t flow_len;
            uint8_t flowgram_format;
            std::vector<char> flow;
            std::vector<char> key;

            /* Validate that common header is well formatted */
            bool validate() const;
            int get_size() const; 
            void host_to_big_endian(); 
            void big_endian_to_host();
    };

    /* Container representing one read header */
    class SFFReadHeader : public VirtualSFFField
    {
        public: 
            uint16_t header_len;
            uint16_t name_len;
            uint32_t nbases;
            uint16_t clip_qual_left;
            uint16_t clip_qual_right;
            uint16_t clip_adapter_left;
            uint16_t clip_adapter_right;
            std::string name;

            /* Virtual methods definition */
            int get_size() const; 
            void host_to_big_endian(); 
            void big_endian_to_host();
    };

    /* Container representing one read data */
    class SFFReadData : public VirtualSFFField
    {
        public: 
            SFFReadData(uint16_t flow_len, uint32_t nbases);
            std::vector<uint16_t> flowgram; 
            std::vector<uint8_t> flow_index;
            std::vector<char> bases;
            std::vector<uint8_t> quality;
            
            /* Virtual methods definition */
            int get_size() const; 
            void host_to_big_endian(); 
            void big_endian_to_host();
    };

    /* Class representing one read. A read is composed of a header
     * and data. Header and data are gathered in the same class
     * because header helps instantiate data. It makes validation
     * easier, too. 
     */
    class SFFField 
    {
        public: 
            SFFField(const SFFFileHeader &header); 
            ~SFFField(); 

            /* Getters, ensuring const-ness */
            const SFFReadHeader* get_header(); 
            const SFFReadData* get_data(); 
            std::string get_name() const; 
            int get_flow_len() const; 
            int get_key_len() const; 
            std::vector<char> get_key() const;
            
            /* Setters */
            void set_header(const SFFReadHeader *header);
            void set_data(const SFFReadData *data); 

            /* Validate the field is well constructed */
            bool validate() const; 
            /* Get clipping values */
            int get_left_clip_value() const;
            int get_right_clip_value() const; 

            std::string get_left_adaptor_sequence(int size) const; 

        private:
            uint16_t key_len;
            uint16_t flow_len;
            std::vector<char> key; 
            const SFFReadHeader *header;
            const SFFReadData *data;
    };
    
    /* Handle IO
     */
    class SFFFileReader
    {
        public: 
            SFFFileReader(const std::string &filename); 
            ~SFFFileReader(); 
            bool read_common_header(SFFFileHeader &header); 
            bool read_field(SFFField &field); 
            bool done(); 
            bool good(); 
        private: 
            bool read_field_header(SFFReadHeader *header); 
            bool read_field_data(SFFReadData *data); 
            bool read_padding(int size); 
            bool validate_common_header(const SFFFileHeader &header); 
            std::ifstream ifs; 
    };

    class SFFFileWriter
    {
        public:
            SFFFileWriter(const std::string &filename); 
            ~SFFFileWriter(); 
            bool write_common_header(const SFFFileHeader &header); 
            bool write_field(SFFField &read); 
            int get_number_of_fields_written(); 
        private: 
            bool write_field_header(const SFFReadHeader *header);
            bool write_field_data(const SFFReadData *data); 
            bool write_padding(int size); 
            std::ofstream ofs;
            int nreads;  // Number of reads we write to file
    };
}
#endif
