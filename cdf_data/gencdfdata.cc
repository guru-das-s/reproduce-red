#include <iostream>
#include <set>
#include <unordered_map>
#include <string>
#include <fstream>

using namespace std;

typedef struct{
  float prob_value;
  int size_value;
}cdfentry_t;

struct classcomp {
  bool operator() (const cdfentry_t& lhs, const cdfentry_t& rhs) const
  {
    return lhs.prob_value < rhs.prob_value; 
  }
};

std::set<cdfentry_t, classcomp> cdf_consecpages;
std::set<cdfentry_t, classcomp> cdf_filesperpage;
std::set<cdfentry_t, classcomp> cdf_primaryreply;
std::set<cdfentry_t, classcomp> cdf_primaryreq;
std::set<cdfentry_t, classcomp> cdf_secondaryreply;
std::set<cdfentry_t, classcomp> cdf_secondaryreq;
std::set<cdfentry_t, classcomp> cdf_thinktimes;

void populate_cdf_data(std::ifstream& file, std::set<cdfentry_t, classcomp>& cdfset)
{
  std::string line;

  if (file.is_open())
  {
    while ( std::getline (file, line) )
    {
      cout << line << '\n';
      size_t found = line.find(",");
      if (found!=std::string::npos)
        std::cout << "Comma found at " << found << '\n';
      std::string prob = line.substr (0,found);
      std::string sizeval = line.substr(found+1);

      cout<<"Prob is: "<<prob<<" and size is: "<<sizeval<<"\n";
      std::string::size_type sz;     // alias of size_t
      cdfentry_t entry;

      entry.prob_value = stof(line, &sz);
      entry.size_value = stoi(line.substr(sz+1));
      cout<<"Entry Prob is: "<<entry.prob_value<<" and Entry Size is: "<<entry.size_value<<"\n";
      cdfset.insert(entry);
    }
    cout<<"\n";
    file.close();
  }
}

int get_size(std::set<cdfentry_t, classcomp>& cdfset, float probval)
{
  cdfentry_t dummy;
  dummy.prob_value = probval;

  return (cdfset.lower_bound(dummy))->size_value;
}

int main()
{
  std::ifstream consecpages_file("p4consecpages.txt");
  std::ifstream filesperpage_file("p4filesperpage.txt");
  std::ifstream primaryreply_file("p4primaryreply.txt");
  std::ifstream primaryreq_file("p4primaryreq.txt");
  std::ifstream secondaryreply_file("p4secondaryreply.txt");
  std::ifstream secondaryreq_file("p4secondaryreq.txt");
  std::ifstream thinktimes_file("p4thinktimes.txt");

  populate_cdf_data(consecpages_file, cdf_consecpages);
  populate_cdf_data(filesperpage_file, cdf_filesperpage);
  populate_cdf_data(primaryreply_file, cdf_primaryreply);
  populate_cdf_data(primaryreq_file, cdf_primaryreq);
  populate_cdf_data(secondaryreply_file, cdf_secondaryreply);
  populate_cdf_data(secondaryreq_file, cdf_secondaryreq);
  populate_cdf_data(thinktimes_file, cdf_thinktimes);

  float probval = 0.55;
  std::cout<<"Size for "<<probval<<" is "<<get_size(cdf_consecpages, probval)<<"\n";

  return 0;
}
