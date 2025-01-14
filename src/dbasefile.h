#pragma once


#include "bstream.h"
#include "bspanutil.h"
#include "bitbang.h"

#include <vector>
#include <map>

// References
// http://web.archive.org/web/20150323061445/http://ulisse.elettra.trieste.it/services/doc/dbase/DBFstruct.htm
// http://independent-software.com/dbase-dbf-dbt-file-format.html#:~:text=A%20.dbf%20file%20consist%20of%20three%20blocks%3A%201,List%20of%20field%20descriptors%203%20List%20of%20records
// 
//

namespace dbf {
	enum class DbfVersion : uint8_t
	{
		Foxbase10				= B8(00000010),			// Foxbase 1.0
		Dbase3					= B8(00000011),			// dBASE III
		Dbase3WithMemo			= B8(10000011),			// dBASE III with memo field
		VisualFoxPro			= B8(00110000),			// Visual FoxPro
		VFPWithAutoIncrement	= B8(00110001),
		VFPWithVarChar			= B8(00110010),
		Dbase4					= B8(01000011),
	};
	
	enum class DbfFieldType : char
	{
		// dBase III field types
		Character		= 'C',
		Date			= 'D',
		Float			= 'F',
		Numeric			= 'N',
		Logical			= 'L',
		
		// Visual FoxPro
		DateTime		= 'T',
		Integer			= 'I',
		Currency		= 'Y',
		
		// dBase III+
		Memo			= 'M',
		
		
		General			= 'G',
		Picture			= 'P',
		VarChar			= 'V',
		Binary			= 'B',
		VarBinary		= 'Q',
		TimeStamp		= '@',
		Double			= 'O',
		AutoIncrement	= '+',
		Unknown			= '?'
	};

	struct DBFFieldDescriptor
	{
		std::string fFieldName{};
		size_t fFieldOffset{ 0 };	// calculated
		
		uint8_t fFieldType{0};
		uint32_t fieldDataAddress{0};
		uint8_t fFieldLength{0};
		uint8_t fieldDecimalCount{ 0 };
		uint16_t fieldWorkAreaID{ 0 };
		uint8_t fExample{ 0 };
		uint8_t fProductionMDXFlag{ 0 };

	
		DbfFieldType kind() const {return static_cast<DbfFieldType>(fFieldType);}
		const std::string& name() const { return fFieldName; }


		size_t size() const { return fFieldLength; }
		
		void offset(const size_t value) { fFieldOffset = value; }
		const size_t offset() const { return fFieldOffset; }

		// Return a ByteSpan representing the raw data of the field
		// The data is not interpreted according to the field type
		const uint8_t* data(const waavs::ByteSpan& rec) const { return (dataSpan(rec).fStart); }
		waavs::ByteSpan dataSpan(const waavs::ByteSpan& rec) const
		{
			// make sure we're not going beyond the bounds of the record
			if (offset() + size() > rec.size())
				return waavs::ByteSpan();

			return waavs::ByteSpan(rec.fStart + offset(), size());
		}

		bool loadFromStream(waavs::BStream& bs)
		{
			if (bs.remaining() < 32)
				return false;

			// Check for end of records array
			if (*bs.data() == 0x0D)
			{
				// if at end, skip past it
				bs.skip(1);
				
				return false;
			}
			
			char fName[11]{0};
			bs.read_copy(fName, 11);
			waavs::ByteSpan nameSpan(fName, 11);
			waavs::chunk_trim(nameSpan, " ");
			fFieldName = std::string(nameSpan.fStart, nameSpan.fEnd);
			

			bs.read_u8(fFieldType);
			bs.read_u32_le(fieldDataAddress);
			bs.read_u8(fFieldLength);
			bs.read_u8(fieldDecimalCount);
			bs.read_u16_le(fieldWorkAreaID);
			bs.read_u8(fExample);
			bs.skip(10);						// reserved
			bs.read_u8(fProductionMDXFlag);

			//printf("DBField: %c  [%3d] %s\n", fFieldType, fFieldLength, fFieldName.c_str());
			
			return true;
		}
	};
	
	struct DBFRecordDescriptor
	{
		size_t fRecordSize{ 0 };
		std::vector< DBFFieldDescriptor> fFields{};
		std::map<std::string, DBFFieldDescriptor> fFieldMap;
		
		const std::vector<DBFFieldDescriptor>& fields() const { return fFields; }
		size_t recordSize() const { return fRecordSize; }	// one less than what the file header reports
															// because file header includes 'delete' byte as well
		
		bool loadFromStream(waavs::BStream& bs)
		{
			// each field descriptor is 32 bytes long
			// If CR is the first byte, then we've reached the end
			// of the field descriptors
			while (bs.remaining() > 0)
			{
				// read the field descriptor
				DBFFieldDescriptor rd{};
				if (!rd.loadFromStream(bs))
					break;

				rd.offset(fRecordSize);	// Set the offset of the field within the record

				fRecordSize += rd.size();
				fFields.push_back(rd);
				fFieldMap[rd.name()] = rd;
			}

			// Done reading field descriptors
			return true;
		}
		
		// get a field descriptor by name
		// returns nullptr if not found
		const DBFFieldDescriptor* getFieldByName(const std::string& name) const
		{
			auto it = fFieldMap.find(name);
			if (it == fFieldMap.end())
				return nullptr;

			return &it->second;
		}

		// retrieve the raw ByteSpan for the field
		// return an empty ByteSpan if the field is not found
		waavs::ByteSpan getFieldData(const waavs::ByteSpan& rec, const std::string& name) const
		{
			const DBFFieldDescriptor* fd = getFieldByName(name);
			if (fd == nullptr)
				return waavs::ByteSpan();

			// We assume the record is positioned at the beginning of the 
			// actual data, and not on the preceding 'delete' byte
			// so we do not need to adjust the offset by 1
			// as it indicates a delete state
			return fd->dataSpan(rec);
		}

		const DBFFieldDescriptor* operator[](const std::string& name) const
		{
			return getFieldByName(name);
		}
	};
	
	//
	// DBFTable
	// Corresponds to version 3 of the DBF file format
	//
	struct DBFTable
	{
		static const uint8_t CR = 0x0D;
		
		std::string fName;
		waavs::ByteSpan fFileSpan{};

		// File Header Information
		uint8_t fVersion{ 0 };
		uint16_t fLastUpdateYear{ 0 };
		uint8_t fLastUpdateMonth{ 0 };
		uint8_t fLastUpdateDay{ 0 };
		uint32_t fNumberOfRecordsInTable{ 0 };
		uint16_t fNumberOfBytesInHeader{ 0 };		// skip to here to be at beginning of records
		uint16_t fNumberOfBytesInRecord{ 0 };


		DBFRecordDescriptor fRecordDescriptor{};
		
		DBFTable(const std::string& name) :fName(name) {}

		// Name property, getter and setter
		const std::string& name() const { return fName; }
		void name(const std::string& name) { fName = name; }
		
		// return fields from fRecordDescriptor
		const std::vector<DBFFieldDescriptor>& fields() const { return fRecordDescriptor.fields(); }
		
		uint8_t version() const { return fVersion; }
		size_t headerSize() const { return fNumberOfBytesInHeader; }
		size_t recordSize() const { return fNumberOfBytesInRecord; }
		size_t recordCount() const { return fNumberOfRecordsInTable; }

		const waavs::ByteSpan& fileSpan() const { return fFileSpan; }
		const DBFRecordDescriptor& recordDescriptor() const { return fRecordDescriptor; }
		
		
		waavs::ByteSpan getRecord(size_t recNum)
		{
			// if the recNum is greater than the number of records in the table, return an empty span
			if (recNum > fNumberOfRecordsInTable)
				return waavs::ByteSpan{};

			//printf("RecordDescriptor Record Size: %zd  BytesInRecord: %zd\n", 
			//	recordDescriptor().recordSize(),
			//	fNumberOfBytesInRecord);

			
			// otherwise, return a span that points to the record
			// this skips the first 'delete' byte of the record
			// and only returns the content
			return waavs::ByteSpan{ fFileSpan.fStart + headerSize() + ((recNum - 1) * fNumberOfBytesInRecord)+1, recordDescriptor().recordSize() };

		}
		
		bool loadFromStream(waavs::BStream& bs)
		{
			if (bs.remaining() < 32)
				return false;

			// Get a span for the whole stream so we can 
			// use that later for record recovery
			fFileSpan = bs.getSpan(bs.remaining());
			
			uint8_t version{ 0 };
			bs.read_u8(version);	// version 3
			fVersion = version & 0x07;
			
			if (fVersion != 3)
				return false;
			
			uint8_t updateYear{ 0 };
			bs.read_u8(updateYear);
			fLastUpdateYear = 1900 + updateYear;
			bs.read_u8(fLastUpdateMonth);
			bs.read_u8(fLastUpdateDay);
			bs.read_u32_le(fNumberOfRecordsInTable);
			bs.read_u16_le(fNumberOfBytesInHeader);
			bs.read_u16_le(fNumberOfBytesInRecord);
			bs.skip(3);			// Reserved bytes 12-14
			bs.skip(13);		// Reserved bytes 15-27, dBASE III+ on a LAN
			bs.skip(4);			// Reserved bytes 28-31
			
			// Read the record descriptors
			bool success =  fRecordDescriptor.loadFromStream(bs);
			
			//printf("END OF HEADER: %zd\n", bs.tell());
			
			return success;
		}


	};



}