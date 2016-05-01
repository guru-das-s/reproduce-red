#include <iostream>
#include <set>
#include <unordered_map>
#include <string>
#include <fstream>

using namespace std;

typedef struct a{
  float prob_value;
  int size_value;
}cdfentry_t;

struct classcomp {
  bool operator() (const cdfentry_t& lhs, const cdfentry_t& rhs) const
  {
    return lhs.prob_value < rhs.prob_value; 
  }
};

set<cdfentry_t, classcomp> probs;

void populate_cdf_data(ifstream& file)
{
  string line;

  if (file.is_open())
  {
    while ( getline (file, line) )
    {
      cout << line << '\n';
      size_t found = line.find(",");
      if (found!=std::string::npos)
        cout << "Comma found at " << found << '\n';
      string prob = line.substr (0,found);
      string sizeval = line.substr(found+1);

      cout<<"Prob is: "<<prob<<" and size is: "<<sizeval<<"\n";
      std::string::size_type sz;     // alias of size_t
      cdfentry_t entry;

      entry.prob_value = stof(line, &sz);
      entry.size_value = stoi(line.substr(sz+1));
      cout<<"Entry Prob is: "<<entry.prob_value<<" and Entry Size is: "<<entry.size_value<<"\n";
      probs.insert(entry);
    }
    cout<<"\n";
    file.close();
  }
}

int main()
{
  ifstream file("p4consecpages.txt");

  populate_cdf_data(file);

  cdfentry_t dummy;
  dummy.prob_value = 0.55;

  cout<<"Size for 0.55 is "<<(probs.lower_bound(dummy))->size_value<<"\n";

  return 0;
}
