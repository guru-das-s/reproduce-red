#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <vector>
#include <string>

using namespace std;

int main()
{
  ifstream file("p4consecpages.txt");
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
    }
    cout<<"\n";
    file.close();
  }

  return 0;
}
