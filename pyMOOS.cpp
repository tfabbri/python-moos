#include "pyMOOS.hpp"

void MOOSExceptionTranslator(const pyMOOSException & e) {
    // Use the Python 'C' API to set up an exception object
    PyErr_SetString(PyExc_RuntimeError, e.what());
}

namespace MOOS {

/** this is a class which wraps MOOS::MOOSAsyncCommClient to provide
 * and interface more suitable for python wrapping
 */
    AsyncCommsWrapper::~AsyncCommsWrapper(){
        Close(true);
    }

    bool AsyncCommsWrapper::Run(const std::string & sServer, int Port, const std::string & sMyName) {
        return BASE::Run(sServer, Port, sMyName, 0);//Comms Tick not used in Async version
    }

    //we can support vectors of objects by not lists so
    //here we have a funtion which copies a list to vector
    MsgVector AsyncCommsWrapper::FetchMailAsVector() {
        MsgVector v;
        MOOSMSG_LIST M;
        if (Fetch(M))
            std::copy(M.begin(), M.end(), std::back_inserter(v));

        return v;
    }

    /* python strings can be binary lets make this specific*/
    bool AsyncCommsWrapper::NotifyBinary(const std::string& sKey, const std::string & sData,
                      double dfTime) {
        CMOOSMsg M(MOOS_NOTIFY, sKey, sData.size(), (void *) sData.data(),
                   dfTime);
        return BASE::Post(M);
    }

    bool AsyncCommsWrapper::on_connect_delegate(void * pParam) {
        MOOS::AsyncCommsWrapper * pMe =
                static_cast<MOOS::AsyncCommsWrapper*> (pParam);
        return pMe->on_connect();
    }
    bool AsyncCommsWrapper::SetOnConnectCallback(bp::object func) {
        BASE: SetOnConnectCallBack(on_connect_delegate, this);
        on_connect_object_ = func;
        return true;
    }

    bool AsyncCommsWrapper::Close(bool nice){
        bool bResult = false;

        Py_BEGIN_ALLOW_THREADS
        //PyGILState_STATE gstate = PyGILState_Ensure();
        closing_ = true;
        bResult = BASE::Close(true);

        //PyGILState_Release(gstate);
        Py_END_ALLOW_THREADS

        return bResult;
    }


    bool AsyncCommsWrapper::on_connect() {

        bool bResult = false;

        PyGILState_STATE gstate = PyGILState_Ensure();
        try {
            bp::object result = on_connect_object_();
            bResult = bp::extract<bool>(result);
        } catch (const bp::error_already_set&) {
            PyErr_Print();
            PyGILState_Release(gstate);
            throw pyMOOSException("OnConnect:: caught an exception thrown in python callback");
        }

        PyGILState_Release(gstate);

        return bResult;
    }

    bool AsyncCommsWrapper::SetOnMailCallback(bp::object func) {
        BASE: SetOnMailCallBack(on_mail_delegate, this);
        on_mail_object_ = func;
        return true;
    }

    bool AsyncCommsWrapper::on_mail_delegate(void * pParam) {
        MOOS::AsyncCommsWrapper * pMe =
                static_cast<MOOS::AsyncCommsWrapper*> (pParam);
        return pMe->on_mail();
    }

    bool AsyncCommsWrapper::on_mail() {
        bool bResult = false;

        PyGILState_STATE gstate = PyGILState_Ensure();
        try {
            if(!closing_){
                bp::object result = on_mail_object_();
                bResult = bp::extract<bool>(result);
            }
        } catch (const bp::error_already_set&) {
            PyErr_Print();
            PyGILState_Release(gstate);
            throw pyMOOSException("OnMail:: caught an exception thrown in python callback");
        }

        PyGILState_Release(gstate);

        return bResult;
    }

    bool AsyncCommsWrapper::active_queue_delegate(CMOOSMsg & M, void* pParam) {
        MeAndQueue * maq = static_cast<MeAndQueue*> (pParam);
        return maq->me_->OnQueue(M, maq->queue_name_);
    }

    bool AsyncCommsWrapper::OnQueue(CMOOSMsg & M, const std::string & sQueueName) {
        std::map<std::string, MeAndQueue*>::iterator q;

        {
            MOOS::ScopedLock L(queue_api_lock_);
            q = active_queue_details_.find(sQueueName);
            if (q == active_queue_details_.end())
                return false;
        }

        bool bResult = false;

        PyGILState_STATE gstate = PyGILState_Ensure();
        try {
            bp::object result = q->second->func_(M);
            bResult = bp::extract<bool>(result);
        } catch (const bp::error_already_set& e) {
            PyGILState_Release(gstate);
            throw pyMOOSException("ActiveQueue:: caught an exception thrown in python callback");
        }

        PyGILState_Release(gstate);

        return bResult;
    }

    bool AsyncCommsWrapper::AddActiveQueue(const std::string & sQueueName, bp::object func) {

        MOOS::ScopedLock L(queue_api_lock_);

        MeAndQueue* maq = new MeAndQueue;
        maq->me_ = this;
        maq->queue_name_ = sQueueName;
        maq->func_ = func;

        std::cerr << "adding active queue OK\n";

        active_queue_details_[sQueueName] = maq;
        return BASE::AddActiveQueue(sQueueName, active_queue_delegate, maq);
    }
};
