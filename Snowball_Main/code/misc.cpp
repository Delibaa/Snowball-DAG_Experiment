#include "misc.h"

vector<string> split(const string &str, const string &delim)
{
  vector<string> tokens;
  size_t prev = 0, pos = 0;
  do
  {
    pos = str.find(delim, prev);
    if (pos == string::npos)
      pos = str.length();
    string token = str.substr(prev, pos - prev);
    // if (!token.empty())
    tokens.push_back(token);
    prev = pos + delim.length();
  } while (pos < str.length() && prev < str.length());
  return tokens;
}

int safe_stoi(string s, bool &pr)
{
  int v;
  try
  {
    v = stoi(s);
  }
  catch (...)
  {
    pr = false;
    return 0;
  }
  pr = pr && true;
  return v;
}

unsigned long safe_stoull(string s, bool &pr)
{
  unsigned long v;
  try
  {
    v = stoull(s);
  }
  catch (...)
  {
    pr = false;
    return 0;
  }
  pr = pr && true;
  return v;
}
