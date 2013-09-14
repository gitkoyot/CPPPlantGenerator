/*
 * Configuration.cpp
 *
 *  Created on: 28-07-2012
 *      Author: koyot
 */
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include "PacketQueue.h"
#include "MessagePacketQueue.h"
#include "Configuration.h"
#include "Exceptions.h"
#include "NetworkDevice.h"
#include <boost/foreach.hpp>

using namespace std;
namespace po = boost::program_options;
////////////////////////////////////////////////////////////////////////////////
namespace LinkDelay
{

ConfigurationInternalData *Configuration::data=nullptr;
Configuration *Configuration::me=nullptr;

Configuration::~Configuration()
{
	if (data)
	{
		delete data;
		data = nullptr;
	}

	PacketQueueSingletonProxy::releaseImp();
}


Configuration* Configuration::instance()
{
	static Configuration configuration;
	if (me==nullptr)
		me = &configuration;
	return me;

}

void Configuration::create(int argc, char* argv[])
{
	if (nullptr!=data)
	{
		delete data;
		data = nullptr;
	}
	if (nullptr==data)
	{
		data = new ConfigurationInternalData();
		data->link_delay=0;
	}

	parseArguments(argc, argv);
	PacketQueueSingletonProxy q;
	q.setImp(new MessagePacketQueue());
	//PacketQueueInterface::setImp(new LockFreePacketQueue());
	q.openOrCreate();
}



void Configuration::parseArguments(int argc, char* argv[])
{

	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
			("help", "produce help message")
			("list,l","list all readable interfaces")
			("iif,i",po::value<string>(&data->input_interface), "set input interface")
			("oif,o",po::value<string>(&data->output_interface), "set output interface")
			("link-delay,m", po::value<int>(&data->link_delay)->default_value(50),"set link delay in ms")
			("custom-mac", po::value<string>(&data->mac_address_string),"set custom source mac address for outgoing packets")
			("input-filter", po::value<string>(&data->input_filter_string),"filter for incoming packets");

	std::stringstream ss;
	ss << desc;

	try
	{
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help"))
		{
			throw ArgParseException() << string_info(ss.str());
			return;
		}

		if (vm.count("list"))
		{
			throw ArgParseException() << string_info(NetworkDevice::getInterfaceList());
		}

		if (!vm.count("iif"))
		{
			throw ArgParseException() << string_info("You need to specify input device" );
		}

		if (!vm.count("oif"))
		{
			throw ArgParseException() << string_info("You need to specify output device" );
		}

		if (vm.count("custom-mac"))
		{
			if (convert_string_to_mac(data->mac_address_string,data->mac_address))
				data->use_custom_mac=true;
			else
				throw ArgParseException() << string_info(string("Invalid mac address: ")+data->mac_address_string);
		}

	} catch (po::error &e)
	{
		throw ArgParseException() << string_info(ss.str() + "\n" + e.what());

	} catch (...)
	{
		throw;
	}

}


bool Configuration::convert_string_to_mac(std::string mac_address_string, char mac_address[])
{
	bool retVal = false;
	boost::regex mac_regex (R"(([0-9a-fA-F]{2}):([0-9a-fA-F]{2}):([0-9a-fA-F]{2}):([0-9a-fA-F]{2}):([0-9a-fA-F]{2}):([0-9a-fA-F]{2}))");
	boost::smatch sm;
	if (boost::regex_match (mac_address_string,sm, mac_regex ))
	{
		  for (unsigned i=1; i<sm.size(); ++i)
		  {
			string octet = sm[i];
			mac_address[i-1] = strtoul ( octet.c_str(), nullptr, 16 );
		  }
		  retVal=true;
	}
	return retVal;

}

boost::exception_ptr* Configuration::getMyOwnException()
{
	boost::exception_ptr *newPtr = new boost::exception_ptr();
	data->exceptions.push_back(std::shared_ptr<boost::exception_ptr>(newPtr) );
	return newPtr;
}

void Configuration::rethrowExceptions()
{
    BOOST_FOREACH( std::shared_ptr<boost::exception_ptr> &ex, data->exceptions )
	{
	  if (*ex.get())
	     boost::rethrow_exception(*ex.get());
	}
}




} /* namespace LinkDelay */

