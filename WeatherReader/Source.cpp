#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>


using namespace std;


const string OUTPUT_FILE_PATH = "D:/output.json";

atomic<bool> stopReadCOM = false;
HANDLE hSerial;
LPCTSTR sPortName = L"COM2";


string getCurrentTime() {
    time_t now = time(nullptr);
    // convert now to string form
    string dt = ctime(&now);
    // convert now to tm struct for UTC
    tm* gmtm = gmtime(&now);
    dt = asctime(gmtm);
    dt.pop_back();

    return dt;
}


string WeatherData2Json(float ws, float wd, bool is_first, string sensorName="WMT700")
{
    stringstream ss;

    if (!is_first)
        ss << ',';

    ss << "{\"time\":\"" << getCurrentTime() << "\",";
    ss << "\"name\":\"" << sensorName << "\",";
    ss << "\"speed\":\"" << ws << "\",";
    ss << "\"direction\":\"" << wd << "\"}";
    std::string json_string = ss.str();

    return json_string;
}



bool extractData(string& input, float& ws, float& wd) {
    size_t start = input.find("$");
    size_t end = input.find("\r\n");

    // Check if found "$" and "\r\n"
    if (start != string::npos && end != string::npos) {
        string data = input.substr(start + 1, end - start - 1);
        size_t comma = data.find(",");

        // Check if found comma ","
        if (comma != string::npos) {
            ws = stof(data.substr(0, comma));
            wd = stof(data.substr(comma + 1));

            return true;
        }
    }

    return false;
}


void readCOM()
{
    ofstream outfile(OUTPUT_FILE_PATH, ios_base::trunc);
    if (!outfile) {
        cerr << "Output file cannot be created or opened\n";
        return;
    }

    DWORD iSize;
    char sReceivedChar;
    string recievedData;
    float wd = 0.0f;
    float ws = 0.0f;
    bool is_first = true;

    outfile << '[';
   
    while (!stopReadCOM)
    {
        try{
            ReadFile(hSerial, &sReceivedChar, 1, &iSize, 0);  // recieved 1bit 

            if (iSize > 0)   // if something recieved
                recievedData += sReceivedChar;

            if (sReceivedChar == '\n') {
                if (extractData(recievedData, ws, wd)) {
                    string json = WeatherData2Json(ws, wd, is_first);
                    outfile << json;
                    outfile.flush();
                    
                    if (is_first)
                        is_first = false;
                }
                recievedData = "";
            }
        }
        catch (...) {
            cerr << "error";
        }  
    }
    outfile << "]";
    outfile.close();
}


void checkConnection() {
    if (hSerial == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            throw runtime_error("serial port does not exist.\n");

        throw runtime_error("error with port connection.\n");
    }
}



void setUpConnection()
{
    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams))
        throw runtime_error("getting state error\n");

    dcbSerialParams.BaudRate = CBR_2400;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams))
        throw runtime_error("error setting serial port state\n");
}

int main()
{
    hSerial = ::CreateFile(sPortName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    try {
        checkConnection();
        setUpConnection();
    }
    catch (runtime_error& error) {
        cerr << error.what();
    }
   
    thread t(readCOM);  
    
    cout << "Press Any Key To Exit...\n";
    getchar();
    cout << "Finishing Program...\n";

    stopReadCOM = true;
    t.join();
    CloseHandle(hSerial);

    cout << "Finished\n";

    return 0;
}