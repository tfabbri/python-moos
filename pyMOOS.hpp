#ifndef PYMOOS_H
#define PYMOOS_H

#include <stdexcept>
#include "MOOS/libMOOS/Comms/MOOSMsg.h"
#include "MOOS/libMOOS/Comms/MOOSCommClient.h"
#include "MOOS/libMOOS/Comms/MOOSAsyncCommClient.h"

// Boost-Python includes
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

namespace bp = boost::python;

typedef std::vector<CMOOSMsg> MsgVector;
typedef std::vector<MOOS::ClientCommsStatus> CommsStatusVector;

struct pyMOOSException : std::runtime_error{
    using std::runtime_error::runtime_error;
};

void MOOSExceptionTranslator(const pyMOOSException & e);

namespace MOOS{
    /** this is a class which wraps MOOS::MOOSAsyncCommClient to provide
     * and interface more suitable for python wrapping
     */
    class AsyncCommsWrapper : public MOOS::MOOSAsyncCommClient {
    private:
        typedef MOOSAsyncCommClient BASE;

        /** this is a structure which is used to dispatch
         * active queue callbacks
         */
        struct MeAndQueue {
            AsyncCommsWrapper* me_;
            std::string queue_name_;
            bp::object func_;
        };
        std::map<std::string, MeAndQueue*> active_queue_details_;
        CMOOSLock queue_api_lock_;

        /** callback functions (stored) */
        bp::object on_connect_object_;
        bp::object on_mail_object_;

        /** close connection flag */
        bool closing_;

    public:

        ~AsyncCommsWrapper();

        bool Run(const std::string & sServer, int Port, const std::string & sMyName);

        //we can support vectors of objects by not lists so
        //here we have a funtion which copies a list to vector
        MsgVector FetchMailAsVector();

        /* python strings can be binary lets make this specific*/
        bool NotifyBinary(const std::string& sKey, const std::string & sData, double dfTime);

        bool SetOnConnectCallback(bp::object func);

        bool Close(bool nice);

        bool on_connect();

        bool on_mail();

        bool SetOnMailCallback(bp::object func);

        bool OnQueue(CMOOSMsg & M, const std::string & sQueueName);

        static bool active_queue_delegate(CMOOSMsg & M, void* pParam);

        static bool on_mail_delegate(void * pParam);

        static bool on_connect_delegate(void * pParam);

        virtual bool AddActiveQueue(const std::string & sQueueName, bp::object func);
    };
};


////////////////////////////////////////////////////////////////////////////////////////////
/**                   here comes the boost python stuff                                   */
////////////////////////////////////////////////////////////////////////////////////////////


BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(f_overloads, f, 1, 2);
BOOST_PYTHON_FUNCTION_OVERLOADS(time_overloads, MOOSTime, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(notify_overloads_2_3, Notify, 2,3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(notify_overloads_3_4, Notify, 3,4);

BOOST_PYTHON_MODULE(pymoos)
{
    bp::docstring_options local_docstring_options(true, true, false);

    PyEval_InitThreads();

    /*********************************************************************
                     MOOSMsg e class
    *********************************************************************/

    bp::class_<CMOOSMsg>("moos_msg","Bar class")
            .def("time", &CMOOSMsg::GetTime)
            .def("trace",&CMOOSMsg::Trace)

            .def("name", &CMOOSMsg::GetKey)
            .def("key", &CMOOSMsg::GetKey)
            .def("is_name", &CMOOSMsg::IsName)

            .def("source", &CMOOSMsg::GetSource)

            .def("is_double", &CMOOSMsg::IsDouble)

            .def("double", &CMOOSMsg::GetDouble)
            .def("double_aux",&CMOOSMsg::GetDoubleAux)

            .def("is_string", &CMOOSMsg::IsString)
            .def("string", &CMOOSMsg::GetString)

            .def("is_binary", &CMOOSMsg::IsBinary)
            .def("binary_data",&CMOOSMsg::GetString)
            .def("binary_data_size",&CMOOSMsg::GetBinaryDataSize)
            .def("mark_as_binary",&CMOOSMsg::MarkAsBinary);

    /*********************************************************************
                     vector of messages
    *********************************************************************/

    bp::class_<MsgVector>("moos_msg_list")
            .def(bp::vector_indexing_suite<MsgVector>());

    /*********************************************************************
                     communications status class
    *********************************************************************/

    bp::class_<MOOS::ClientCommsStatus>("moos_comms_status")
            .def("appraise",&MOOS::ClientCommsStatus::Appraise)
            .def("print",&MOOS::ClientCommsStatus::Write);

    /*********************************************************************
                     vector of communications status classes
    *********************************************************************/

    bp::class_<CommsStatusVector>("moos_comms_status_list")
            .def(bp::vector_indexing_suite<CommsStatusVector>());

    /*********************************************************************
                     comms base class
    *********************************************************************/

    bp::class_<CMOOSCommObject>("base_comms_object", bp::no_init);

    /*********************************************************************
                     synchronous comms base class
    *********************************************************************/
    bp::class_<CMOOSCommClient, bp::bases<CMOOSCommObject>, boost::noncopyable>("base_sync_comms",bp::no_init)

        .def("register",static_cast<bool(CMOOSCommClient::*)(const std::string&, double)> (&CMOOSCommClient::Register))
        .def("register",static_cast<bool(CMOOSCommClient::*)(const std::string&,const std::string&,double)> (&CMOOSCommClient::Register))
        .def("is_registered_for",&CMOOSCommClient::IsRegisteredFor)

        .def("notify",static_cast<bool(CMOOSCommClient::*)(const std::string&,const std::string&, double)> (&CMOOSCommClient::Notify),notify_overloads_2_3())
        .def("notify",static_cast<bool(CMOOSCommClient::*)(const std::string&,const std::string&,const std::string&,double)> (&CMOOSCommClient::Notify),notify_overloads_2_3())
        .def("notify",static_cast<bool(CMOOSCommClient::*)(const std::string&,const char *,double)> (&CMOOSCommClient::Notify))
        .def("notify",static_cast<bool(CMOOSCommClient::*)(const std::string&,const char *,const std::string&,double)> (&CMOOSCommClient::Notify))
        .def("notify",static_cast<bool(CMOOSCommClient::*)(const std::string&,double,double)> (&CMOOSCommClient::Notify))
        .def("notify",static_cast<bool(CMOOSCommClient::*)(const std::string&,double,const std::string&,double)> (&CMOOSCommClient::Notify))

        .def("get_community_name", &CMOOSCommClient::GetCommunityName)
        .def("get_moos_name",&CMOOSCommClient::GetMOOSName)

        .def("close", &CMOOSCommClient::Close)

        .def("get_published", &CMOOSCommClient::GetPublished)
        .def("get_registered",&CMOOSCommClient::GetRegistered)
        .def("get_description",&CMOOSCommClient::GetDescription)

        .def("is_running", &CMOOSCommClient::IsRunning)
        .def("is_asynchronous",&CMOOSCommClient::IsAsynchronous)
        .def("is_connected",&CMOOSCommClient::IsConnected)
        .def("wait_until_connected",&CMOOSCommClient::WaitUntilConnected)

        .def("get_number_of_unread_messages",&CMOOSCommClient::GetNumberOfUnreadMessages)

        .def("get_number_of_unsent_messages",&CMOOSCommClient::GetNumberOfUnsentMessages)
        .def("get_number_bytes_sent",&CMOOSCommClient::GetNumBytesSent)
        .def("get_number_bytes_read",&CMOOSCommClient::GetNumBytesReceived)
        .def("get_number_messages_sent",&CMOOSCommClient::GetNumMsgsSent)
        .def("get_number_message_read",&CMOOSCommClient::GetNumMsgsReceived)
        .def("set_comms_control_timewarp_scale_factor",&CMOOSCommClient::SetCommsControlTimeWarpScaleFactor)
        .def("get_comms_control_timewarp_scale_factor", &CMOOSCommClient::GetCommsControlTimeWarpScaleFactor)
        .def("do_local_time_correction",&CMOOSCommClient::DoLocalTimeCorrection)

        .def("set_verbose_debug", &CMOOSCommClient::SetVerboseDebug)
        .def("set_comms_tick",&CMOOSCommClient::SetCommsTick)
        .def("set_quiet",&CMOOSCommClient::SetQuiet)

        .def("enable_comms_status_monitoring",&CMOOSCommClient::EnableCommsStatusMonitoring)
        .def("get_client_comms_status",&CMOOSCommClient::GetClientCommsStatus)
        .def("get_client_comms_statuses",&CMOOSCommClient::GetClientCommsStatuses);


    /*********************************************************************
                     Asynchronous comms base class
    *********************************************************************/

    bp::class_<MOOS::MOOSAsyncCommClient, bp::bases<CMOOSCommClient>,boost::noncopyable>("base_async_comms", bp::no_init)
            .def("run",&MOOS::MOOSAsyncCommClient::Run);

    /*********************************************************************/
    /** FINALLY HERE IS THE CLASS WE EXPECT TO BE INSTANTIATED IN PYTHON */
    /*********************************************************************/
    //this is the one to use
    bp::class_<MOOS::AsyncCommsWrapper, bp::bases<MOOS::MOOSAsyncCommClient>,boost::noncopyable>("comms")

        .def("run", &MOOS::AsyncCommsWrapper::Run)
        .def("close", &MOOS::AsyncCommsWrapper::Close)

        .def("fetch",&MOOS::AsyncCommsWrapper::FetchMailAsVector)
        .def("set_on_connect_callback",&MOOS::AsyncCommsWrapper::SetOnConnectCallback)
        .def("set_on_mail_callback",&MOOS::AsyncCommsWrapper::SetOnMailCallback)
        .def("notify_binary",&MOOS::AsyncCommsWrapper::NotifyBinary)
        .def("add_active_queue",&MOOS::AsyncCommsWrapper::AddActiveQueue)
        .def("remove_message_route_to_active_queue",&MOOS::AsyncCommsWrapper::RemoveMessageRouteToActiveQueue)
        .def("remove_active_queue",&MOOS::AsyncCommsWrapper::RemoveActiveQueue)
        .def("has_active_queue",&MOOS::AsyncCommsWrapper::HasActiveQueue)
        .def("print_message_to_active_queue_routing",&MOOS::AsyncCommsWrapper::PrintMessageToActiveQueueRouting)
        .def("add_message_route_to_active_queue",static_cast<bool(CMOOSCommClient::*)(const std::string &,const std::string &)> (&MOOS::AsyncCommsWrapper::AddMessageRouteToActiveQueue));


    /*********************************************************************
                    some MOOS Utilities
    *********************************************************************/

    /** here are some global help functions */
    bp::def("time", &MOOSTime, time_overloads());
    bp::def("local_time", &MOOSLocalTime, time_overloads());
    bp::def("is_little_end_in", &IsLittleEndian);
    bp::def("set_moos_timewarp", &SetMOOSTimeWarp);
    bp::def("get_moos_timewarp", &GetMOOSTimeWarp);

    bp::register_exception_translator<pyMOOSException>(&MOOSExceptionTranslator);
}
#endif // PYMOOS_H
