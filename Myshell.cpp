#include <iostream>

#include <string>

#include <fstream>



int main() {



    std::cout << std::unitbuf;

    std::cerr << std::unitbuf;



    //std::cout << "$ ";



    const char* home=std::getenv("HOME");

    std::string historyPath=std::string(home)+"/.kubsh_history";



    std::string input;

    while (std::getline(std::cin, input))

    {

        

        if(!input.empty()){

	    std::ofstream history(historyPath,std::ios::app);

	    history << input<<"\n";

 	}

	if(input=="history")

	{

	    std::ifstream historyOutput(historyPath);

            std::string line;

            while(std::getline(historyOutput,line))

            {

                std::cout<<line<<"\n";

            }

	}

        else if (input == "\\q"){

            break;
        }

        else if (input.substr(0, 7) == "debug '" && input[input.length() - 1] == '\'')

        {

        std::cout << input.substr(7, input.length() - 8) << std::endl;    

        }

else if (input.substr(0,4) == "\\e $")
{
    std::string varName = input.substr(4);
    const char* value = std::getenv(varName.c_str());

   if(value != nullptr)
    {
        std::string valueStr = value;
        
        bool has_colon = false;
        for (char c : valueStr)
        {
            if (c == ':') 
            {
                has_colon = true;
                break;
            }
        }
        
        if (has_colon) 
        {
            std::string current_part = "";
            for (char c : valueStr)
            {
                if (c == ':') 
                {
                    std::cout << current_part << "\n";
                    current_part = "";
                }
                else 
                {
                    current_part += c;
                }
            }
            std::cout << current_part << "\n";
        }
        else 
        { 
            std::cout << valueStr << "\n";
        }
    }
    else
    {
        std::cout << varName << ": не найдено\n";
    }
}



	else std::cout<<input<<": command not found\n";


	//std::cout<<"$ ";

    }

}
