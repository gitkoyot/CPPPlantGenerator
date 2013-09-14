/*
 * Configuration.h
 *
 *  Created on: 28-07-2012
 *      Author: koyot
 */

#ifndef INITIALIZER_H_
#define INITIALIZER_H_

#include <boost/thread.hpp>
#include <atomic>

class ThisIsAssociatedClass
{
};

namespace LinkDelay
{
////////////////////////////////////////////////////////////////////////////////
struct ConfigurationInternalData
{
	std::string input_interface;
	std::string output_interface;
	std::string mac_address_string;
	std::string input_filter_string;
	char mac_address[6];
	bool use_custom_mac = false;
	int link_delay;
	int verbose_flag;

	/**
	 * detemines if program is allowed to kill itsself :P
	 */
	bool allowed_to_finish;

	std::vector<std::shared_ptr<boost::exception_ptr> > exceptions;

	ConfigurationInternalData() :
			input_interface(""),
			output_interface(""),
			mac_address_string(""),
			input_filter_string(""),
			mac_address	{ 0, 0, 0, 0, 0, 0 },
			use_custom_mac(false),
			link_delay(50),
			verbose_flag(false),
			allowed_to_finish(false)
	{

	}

};
////////////////////////////////////////////////////////////////////////////////
class Configuration
{
private:
	/**
	 * parse argc/argv and throw exception if something fails.
	 * @param argc
	 * @param argv
	 */
	static void parseArguments(int argc, char* argv[]);

	static bool convert_string_to_mac(std::string mac_address_string,
			char mac_address[]);

	static std::string getInterfaceList();

protected:
	static ConfigurationInternalData data_static_composition;
	static ConfigurationInternalData* data;
	static Configuration *me;

public:

	static void create(int argc, char* argv[]);

	static Configuration* instance();


	virtual ~Configuration();

	/**
	 * function creates boost:exception_ptr for thread and adds it to internal
	 * exception vector
	 * @return exception_ptr for use inside thread
	 */

	boost::exception_ptr* getMyOwnException();

	/**
	 * Rethrows any exceptions stored in exception vector.
	 */
	void rethrowExceptions();

	void function(ThisIsAssociatedClass* dummy);

public:

	const std::string& getOutputInterface()
	{
		return data->output_interface;
	}

	const std::string& getInputInterface()
	{
		return data->input_interface;
	}

	const std::string& getInputFilterString()
	{
		return data->input_filter_string;
	}


	/**
	 * Determines if all threads may exit
	 * @return
	 */
	bool may_i_die()
	{
		return data->allowed_to_finish;
	}

	/**
	 * This function returns proper interface to listen on/write to
	 * @return
	 */
	virtual const std::string getInterface() const
	{
		return std::string();
	}

	virtual const std::string getFilterString() const
	{
		return std::string();
	}

	/**
	 * Sets error condition by which all threads know when to finish procesing
	 */
	void fatal_error()
	{
		data->allowed_to_finish = true;
	}

	void setAllowedToFinish(bool allowedToFinish)
	{
		data->allowed_to_finish = allowedToFinish;
	}

	bool getUseCustomMac()
	{
		return data->use_custom_mac;
	}

	int getLinkDelay()
	{
		return data->link_delay;
	}

	const char* getMacAddress()
	{
		return data->mac_address;
	}

};
////////////////////////////////////////////////////////////////////////////////
} /* namespace LinkDelay */
#endif /* INITIALIZER_H_ */
