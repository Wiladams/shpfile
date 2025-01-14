#pragma once

#include "dbasefile.h"
#include "bspanutil.h"

#include <vector>
#include <map>

namespace dbfutil {
	void printDbfRecordDescriptor(const dbf::DBFRecordDescriptor& rd)
	{
		printf("***********************************\n");
		printf(" Dbf Record Size: %zd\n", rd.recordSize());
		printf("***********************************\n");

		for (auto& field : rd.fields())
		{
			printf("Field Name: %s\n", field.name().c_str());
			printf("Field Type: %c\n", field.kind());
			printf("Field Size: %zd\n", field.size());
			printf("============================================\n");
		}
	}


	void printDbfFileDetail(dbf::DBFTable& dbf)
	{
		printf("******************************\n");
		printf(" Header Size: %zd\n", dbf.headerSize());
		printf(" Record Count: %zd\n", dbf.recordCount());
		printf(" Record Size: %zd\n", dbf.recordSize());

		printf("----------- FIELDS -----------\n");
		printDbfRecordDescriptor(dbf.recordDescriptor());

		printf("------------------------------\n");

		printf("=========== DATA =============\n");
		for (size_t recNum = 1; recNum <= dbf.recordCount(); recNum++)
		{
			auto rec = dbf.getRecord(recNum);

			waavs::printChunk(rec);
		}
	}

	
	// Print a CSV file from Dbf
	void outputDbfFieldsHeaderCSV(const dbf::DBFRecordDescriptor& rd, FILE *o)
	{
		bool firstOne = true;
		for (auto& field : rd.fields())
		{
			if (!firstOne)
			{
				fprintf(o,", ");
			}
			else
				firstOne = false;

			fprintf(o,"%s", field.name().c_str());
		}
		fprintf(o,"\n");
	}

	// write a chunk to a CSV file, wrapping in '"' if needed
	// this will happen if the entry has a ',' character in it
	static void outputCSVChunk(const waavs::ByteSpan& chunk, FILE *o)
	{
		bool useQuote = false;
		if (waavs::chunk_find_char(chunk, ','))
			useQuote = true;

		waavs::ByteSpan s = chunk;

		if (useQuote)
			fprintf(o,"\"");

		while (s && *s) {
			fprintf(o,"%c", *s);
			s++;
		}

		if (useQuote)
			fprintf(o,"\"");
	}

	/*
	static void writeCSVChunk(const waavs::ByteSpan& chunk)
	{
		bool useQuote = false;
		if (waavs::chunk_find_char(chunk, ','))
			useQuote = true;
		
		waavs::ByteSpan s = chunk;

		if (useQuote)
			printf("\"");
		
		while (s && *s) {
			printf("%c", *s);
			s++;
		}

		if (useQuote)
			printf("\"");
	}
	*/
	
	void outputDbfRecordCSV(const dbf::DBFRecordDescriptor& rd, const waavs::ByteSpan& inChunk, FILE *o)
	{
		// Create a stream to traverse the rec
		waavs::ByteSpan rec(inChunk);

		bool firstOne = true;
		for (auto& field : rd.fields())
		{
			if (!firstOne)
			{
				fprintf(o, ",");
			}
			else
				firstOne = false;

			auto value = rd.getFieldData(rec, field.name());

			value = waavs::chunk_trim(value, " \0");	// trim whitespace and nulls
			outputCSVChunk(value, o);
		}
		fprintf(o, "\n");
	}

	void outputDbfFileCSV(dbf::DBFTable& dbf, FILE *o)
	{
		// Print the header row with column names
		outputDbfFieldsHeaderCSV(dbf.recordDescriptor(), o);

		// Print the row data
		for (size_t recNum = 1; recNum <= dbf.recordCount(); recNum++)
		{
			auto rec = dbf.getRecord(recNum);
			outputDbfRecordCSV(dbf.recordDescriptor(), rec, o);
		}
	}

}
