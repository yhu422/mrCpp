//#include "task.grpc.pb.h"
//#include <iostream>
//#include <memory>
//#include <string>
//
//#include "absl/flags/flag.h"
//#include "absl/flags/parse.h"
//
//#include <grpcpp/grpcpp.h>
//using grpc::Channel;
//using grpc::ClientContext;
//using grpc::Status;
//using task::SendingServices;
//using task::Task;
//using task::TaskReply;
//
// Created by Yile Hu on 10/14/24.
//
#include <memory>
#include <string>
#include <queue>
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include "task.grpc.pb.h"
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using task::MRCoordinator;
using task::Empty;
using task::FinishTaskRequest;
using task::PollJobReply;
using task::TaskReply;
using task::Job;
using task::JobID;
ABSL_FLAG(uint16_t, port, 50051, "Server port for the service");


struct JobData{
    std::vector<std::string> files;
    std::string out_dir;
    uint32_t num_reduce_task;
    uint32_t num_reduce_completed;
    uint32_t num_map_task;
    uint32_t num_map_completed;
    uint32_t next_map_id;
    uint32_t next_reduce_id;
    bool is_completed;
    bool is_failed;
};

class MRCoordinatorImpl final : public MRCoordinator::Service {
private:
    uint32_t _nextJobID = 0;
    //List of Uncompleted jobs, used a queue so jobs are executed in a round robin fashion
    std::list<uint32_t> _jobIDList;
    std::unordered_map<uint32_t, JobData> _jobInfoTable;
public:
    Status GetTask(ServerContext* context, const Empty* task,
                    TaskReply* reply) override {
        if(_jobIDList.empty()){
            reply->set_wait(true);
            return Status::OK;
        }else{
            reply->set_wait(false);
        }
        uint32_t jobID = _jobIDList.front();
        reply->set_job_id(jobID);
        _jobIDList.pop_front();
        _jobIDList.push_back(jobID);
        JobData& jd = _jobInfoTable[jobID];
        if(jd.num_map_completed != jd.num_map_task){
            reply->set_is_reduce(false);
            reply->set_task_id(jd.next_map_id);
            reply->set_output_dir(jd.out_dir);
            reply->set_file(jd.files[jd.next_map_id]);
            jd.next_map_id++;
        }else{
            reply->set_is_reduce(true);
            reply->set_task_id(jd.next_reduce_id);
            reply->set_output_dir(jd.out_dir);
            jd.next_reduce_id++;
        }
        return Status::OK;
    }

    Status SubmitJob(ServerContext* context, const Job* job,
                     JobID* reply) override{
        JobData jd;
        jd.files = std::vector<std::string>(job->file_list().begin(), job->file_list().end());
        jd.out_dir = job->output_dir();
        jd.num_reduce_task = job->n_reduce();
        jd.num_reduce_completed = 0;
        jd.next_reduce_id = 0;
        jd.num_map_task = jd.files.size();
        jd.num_map_completed = 0;
        jd.next_map_id = 0;
        _jobIDList.push_back(_nextJobID);
        _jobInfoTable[_nextJobID] = jd;
        reply->set_job_id(_nextJobID++);
        std::cout << "Received Job" << std::endl;
        return Status::OK;
    }
    Status FinishTask(ServerContext* context, const FinishTaskRequest* req, Empty* reply) override{
        if(req->success()){
            JobData& jd = _jobInfoTable[req->job_id()];
            if(req->is_reduce()){
                jd.num_reduce_completed++;
            }else{
                jd.num_map_completed++;
            }
            //The job has been completed;
            if(jd.num_reduce_completed == jd.num_reduce_task){
                jd.is_completed = true;
                _jobIDList.remove(req->job_id());
            }
        }else{

        }
        return Status::OK;
    }

    Status PollJob(ServerContext* context, const JobID* req, PollJobReply* reply) override{
        uint32_t job_id = req->job_id();
        if(_jobInfoTable.count(job_id) == 0){
            reply->set_invalid(true);
        }else{
            reply->set_invalid(false);
        }
        JobData& jd = _jobInfoTable[job_id];
        reply->set_done(jd.is_completed);
        reply->set_failed(jd.is_failed);
        return Status::OK;
    }
};


//class CoordinatorStub{
//public:
//    CoordinatorStub(std::shared_ptr<Channel> channel)
//            : stub_(SendingServices::NewStub(channel)) {}
//
//    // Assembles the client's payload, sends it and presents the response back
//    // from the server.
//    float SendTask(float input) {
//        // Data we are sending to the server.
//        Task request;
//        request.set_x(input);
//
//        // Container for the data we expect from the server.
//        TaskReply reply;
//
//        // Context for the client. It could be used to convey extra information to
//        // the server and/or tweak certain RPC behaviors.
//        ClientContext context;
//
//        // The actual RPC.
//        Status status = stub_->SendTask(&context, request, &reply);
//
//        // Act upon its status.
//        if (status.ok()) {
//            return reply.x();
//        } else {
//            std::cout << status.error_code() << ": " << status.error_message()
//                      << std::endl;
//            return -1;
//        }
//    }
//
//private:
//    std::unique_ptr<SendingServices::Stub> stub_;
//};
void RunServer() {
    std::string server_address("0.0.0.0:50051");
    MRCoordinatorImpl service;
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}
int main(int argc, char* argv[]){
    absl::ParseCommandLine(argc, argv);
    RunServer();
    return 0;
};