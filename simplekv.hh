///////////////////////////////////////////////////////////////////////////////
//
// Simple Key Values - Version 0.0.1
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SKV_NEW
	#define SKV_NEW(x)			reinterpret_cast<x*>(malloc(sizeof(x)))
#endif
#ifndef SKV_FREE
	#define SKV_FREE(x)			free(x)
#endif

class qSimpleKV
{
public:
	const char* mKey = 0;
	const char* mValue = 0;

	qSimpleKV* mNext = 0;
	qSimpleKV* mChild = 0;

	~qSimpleKV()
	{
		RecursiveFree(this);
	}

	qSimpleKV* FindKey(const char* name) const
	{
		for (auto kv = mChild; kv; kv = kv->mNext)
		{
			if (strcmp(kv->mKey, name) == 0) {
				return kv;
			}
		}

		return 0;
	}

protected:
	qSimpleKV* CreateKeyValue()
	{
		auto kv = SKV_NEW(qSimpleKV);
		if (kv) {
			memset(kv, 0, sizeof(kv));
		}

		return kv;
	}

	char* FindUnescapedQuote(char* buf)
	{
		for (char* c = buf; *c; ++c)
		{
			if (*c == '\\')
			{
				++c;
				continue;
			}

			if (*c == '\"') {
				return c;
			}
		}

		return 0;
	}

	void RecursiveFree(qSimpleKV* kv)
	{
		for (auto n = kv->mChild; n;)
		{
			if (n->mChild) {
				RecursiveFree(n);
			}

			auto next = n->mNext;

			SKV_FREE(n);

			n = next;
		}
	}

	void WriteIndents(FILE* file, int n)
	{
		for (int i = 0; n > i; ++i) {
			fwrite("\t", 1, 1, file);
		}
	}

	void RecursiveSaveToFile(qSimpleKV* kv, FILE* file, int level)
	{
		WriteIndents(file, level);
		fprintf(file, "\"%s\"\n", kv->mKey);

		WriteIndents(file, level);
		fprintf(file, "{\n");

		for (auto n = kv->mChild; n; n = n->mNext)
		{
			if (n->mChild)
			{
				RecursiveSaveToFile(n, file, level + 1);
				continue;
			}

			if (n->mKey && n->mValue)
			{
				WriteIndents(file, level + 1);
				fprintf(file, "\"%s\"\t\t\"%s\"\n", n->mKey, n->mValue);
			}
		}

		WriteIndents(file, level);
		fprintf(file, "}\n");
	}

	bool RecursiveLoad(qSimpleKV* kv, char* buf, char** buf_step)
	{
		while (1)
		{
			char c = *buf;
			if (c == 0) {
				return 1;
			}

			// Skip this tag for now...
			if (c == '#')
			{
				while (*buf++ != '\n') {}

				continue;
			}

			// Skip new line & white spaces.

			if (c == '\n' || c == ' ' || c == '\t')
			{
				++buf;
				continue;
			}

			if (c == '\"')
			{
				if (!kv->mKey)
				{
					kv->mKey = ++buf;

					if (auto kend = FindUnescapedQuote(buf))
					{
						*kend = 0;
						buf = ++kend;
						continue;
					}

					return 0;
				}

				if (!kv->mValue)
				{
					if (!kv->mChild)
					{
						kv->mValue = ++buf;

						if (auto kend = FindUnescapedQuote(buf))
						{
							*kend = 0;
							buf = ++kend;
						}
					}

					kv->mNext = CreateKeyValue();
					if (!kv->mNext) {
						return 0;
					}

					return RecursiveLoad(kv->mNext, buf, buf_step);
				}

				return 0;
			}

			if (c == '{')
			{
				++buf;

				kv->mChild = CreateKeyValue();
				if (!kv->mChild) {
					return 0;
				}

				if (!RecursiveLoad(kv->mChild, buf, &buf)) {
					return 0;
				}

				continue;
			}

			if (c == '}')
			{
				if (buf_step) {
					*buf_step = ++buf;
				}

				return 1;
			}
		}

		return 1;
	}

public:

	bool SaveToFile(const char* filename)
	{
		auto file = fopen(filename, "w");
		if (!file) {
			return 0;
		}

		RecursiveSaveToFile(this, file, 0);

		fclose(file);
		return 1;
	}

	/*
	*	Parses KeyValues from the given buffer.
	*	The function will modify buffer and use it for the KeyValues.
	*	Do NOT free the buffer while working with the KeyValues.
	*	After you're done with the KeyValues, you're responsible for freeing the buffer.
	*/
	bool LoadFromBuffer(char* buf)
	{
		return RecursiveLoad(this, buf, 0);
	}
};

///////////////////////////////////////////////////////////////////////////////

class qSimpleKVBuffer : public qSimpleKV
{
public:
	void* mBuffer = 0;

	~qSimpleKVBuffer()
	{
		if (mBuffer) {
			SKV_FREE(mBuffer);
		}
	}

	bool LoadFromBuffer(char* buf)
	{
		mBuffer = buf;

		return RecursiveLoad(this, buf, 0);
	}
};