
#ifndef _FIELD_FIXED_H_
#define _FIELD_FIXED_H_

#include "Field.h"

namespace EmbeddedProto
{

  namespace Detail 
  {
    //! The field type with fixed length.
    template<WireType WIRE_TYPE, class DATA_TYPE>
    class FieldFixed : public Field
    {
      protected:

        //! Check if for a fixed field a valid wire type has been supplied.
        static_assert((WIRE_TYPE == WireType::FIXED64) || (WIRE_TYPE == WireType::FIXED32), 
                "Invalid wire type supplied in template, only FIXEDXX types are allowed.");

        //! This typedef will return an unsigned 32 or 64 variable depending on the field type.
        typedef typename std::conditional<WIRE_TYPE == WireType::FIXED32, uint32_t, uint64_t>::type 
                                                                                  VAR_UINT_TYPE;

        //! This typedef will return a signed 32 or 64 variable depending on the field type.
        typedef typename std::conditional<WIRE_TYPE == WireType::FIXED32, int32_t, int64_t>::type 
                                                                                  VAR_INT_TYPE;
        
        //! The Protobuf default value for this type. Zero in this case.
        static constexpr DATA_TYPE DEFAULT_VALUE = static_cast<DATA_TYPE>(0);

        //! The number of bytes in this fixed field.
        static constexpr uint32_t N_BYTES_IN_FIXED = std::numeric_limits<VAR_UINT_TYPE>::digits 
                                                          / std::numeric_limits<uint8_t>::digits;

      public:

        //! Constructor which also sets the field id number of this field.
        /*!
            \param[in] The field id number as specified in the *.proto file.
        */
        FieldFixed(const uint32_t number) :
            Field(WIRE_TYPE, number),
            _data(DEFAULT_VALUE)
        {

        }

        //! The constructor is default as we do not have members with non standart memory allocation.
        ~FieldFixed() final = default;

        //! Set the value of this field
        void set(const DATA_TYPE& value)
        {
          _data = value;
        }

        //! Assignment opertor to set the data.
        FieldFixed& operator=(const DATA_TYPE& value)
        {
          set(value);
          return *this;
        }

        //! Obtain the value of this field.
        const DATA_TYPE& get() const
        {
          return _data; 
        }

        //! \see Field::serialize()
        Result serialize(MessageBufferInterface& buffer) const final
        {
          Result result(Result::OK);
          // Only serialize if the data does not equal the default and when the size in the buffer 
          // is sufficient.
          // This if statement would be more comperhensive if we where using C++17 std::abs.
          if(!(((DEFAULT_VALUE + std::numeric_limits<DATA_TYPE>::epsilon()) >= _data) &&
               ((DEFAULT_VALUE - std::numeric_limits<DATA_TYPE>::epsilon()) <= _data)))
          {
            if(serialized_size() <= buffer.get_max_size()) 
            {
              _serialize_varint(tag(), buffer);
              const void* pVoid = static_cast<const void*>(&_data);
              const VAR_UINT_TYPE* pUInt = static_cast<const VAR_UINT_TYPE*>(pVoid);
              _serialize_fixed(*pUInt, buffer);            }
            else
            {
              result = Result::ERROR_BUFFER_TO_SMALL;
            }
            
          }
          return result;
        }

        //! \see Field::deserialize()
        Result deserialize(MessageBufferInterface& buffer) final
        {
          // Check if there is enough data in the buffer for a fixed value.
          bool result = N_BYTES_IN_FIXED <= buffer.get_size();
          VAR_UINT_TYPE d = 0;
          result = result && _deserialize_fixed(d, buffer);
          if(result) {
            const void* pVoid = static_cast<const void*>(&d);
            const DATA_TYPE* pData = static_cast<const DATA_TYPE*>(pVoid);
            _data = *pData;
          }
          return result ? Result::OK : Result::ERROR_BUFFER_TO_SMALL;
        }

        //! \see Field::clear()
        void clear() final
        {
          _data = DEFAULT_VALUE;
        }

        //! \see Field::serialized_data_size()
        uint32_t serialized_data_size() const final
        {
          return N_BYTES_IN_FIXED;
        }


      protected:

        //! Serialize a single unsigned integer into the buffer using the fixed size method.
        /*!
            The fixed size method just pushes every byte into the buffer.

            \param[in] value The variable to serialize.
            \param[in,out] buffer The data buffer to which the variable is serialized.
            \return True when serialization succedded.
        */
        bool _serialize_fixed(VAR_UINT_TYPE value, MessageBufferInterface& buffer) const
        {
          // Write the data little endian to the buffer.
          // TODO Define a little endian flag to support memcpy the data to the buffer.

          bool result(true);
          for(uint8_t i = 0; i < std::numeric_limits<VAR_UINT_TYPE>::digits; 
              i += std::numeric_limits<uint8_t>::digits)  
          {
            result = buffer.push(static_cast<uint8_t>((value >> i) & 0x00FF));
            if(!result)
            {
              // No more space in the buffer.
              break;
            }
          }
          return result;
        }

        //! Deserialize a fixed length value from the buffer into the given variable.
        /*!
            \param[out] value The variable in which the result is returned.
            \param[in,out] buffer The buffer from which to deserialize the data.
            \return True when deserialization succedded.
        */
        bool _deserialize_fixed(VAR_UINT_TYPE& value, MessageBufferInterface& buffer) const
        {
          // Read the data little endian to the buffer.
          // TODO Define a little endian flag to support memcpy the data from the buffer.

          VAR_UINT_TYPE temp_value = 0;
          bool result(true);
          uint8_t byte = 0;
          for(uint8_t i = 0; i < std::numeric_limits<VAR_UINT_TYPE>::digits; 
              i += std::numeric_limits<uint8_t>::digits)  
          {
            result = buffer.pop(byte);
            if(result)
            {
              temp_value |= static_cast<VAR_UINT_TYPE>(byte) << i;
            }
            else
            {
              // End of buffer
              break;
            }
          }

          if(result)
          {
            value = temp_value;
          }

          return result;
        }


      private:

        //! The actual data.
        DATA_TYPE _data;

    };

  } // End of namespace detail


  /*!
    Actually define the types to beused in messages. These specify the template for different field 
    types.
    \{
  */
  typedef Detail::FieldFixed<WireType::FIXED64, uint64_t> fixed64;
  typedef Detail::FieldFixed<WireType::FIXED64, int64_t>  sfixed64;
  typedef Detail::FieldFixed<WireType::FIXED64, double>   double64;

  typedef Detail::FieldFixed<WireType::FIXED32, uint32_t> fixed32;
  typedef Detail::FieldFixed<WireType::FIXED32, int32_t>  sfixed32;
  typedef Detail::FieldFixed<WireType::FIXED32, float>    float32;
  /*! \} */

} // End of namespace EmbeddedProto

#endif