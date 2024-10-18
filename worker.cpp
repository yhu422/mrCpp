//
// Created by Yile Hu on 10/14/24.
//
#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include "task.grpc.pb.h"
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::Status;
using task::MRCoordinator;
using task::TaskReply;
using task::Empty;
using task::FinishTaskRequest;
//
// Created by Yile Hu on 10/14/24.
//

void example_map(const std::string& key){
    sleep(1);
    std::cout << key << std::endl;
}
void example_reduce(){
    sleep(1);
}
class MRWorker {
public:
    MRWorker(std::shared_ptr<Channel> channel)
    : stub_(MRCoordinator::NewStub(channel)) {};

    Status GetTask(TaskReply& reply){
        Empty e;
        ClientContext context;
        Status status = stub_->GetTask(&context, e, &reply);
        if(!status.ok()){
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
        }
        return status;
    }
    Status FinishTask(uint32_t job_id, uint32_t task_id, bool is_reduce, bool success){
        FinishTaskRequest req;
        ClientContext context;
        req.set_task_id(task_id);
        req.set_job_id(job_id);
        req.set_is_reduce(is_reduce);
        req.set_success(success);
        Empty e;
        Status status = stub_->FinishTask(&context, req, &e);
        if(!status.ok()){
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
        }
        return status;
    }
private:
    std::unique_ptr<MRCoordinator::Stub> stub_;
};
int main(int argc, char* argv[]){
    absl::ParseCommandLine(argc, argv);
    MRWorker worker(grpc::CreateChannel("localhost:50051",
                                        grpc::InsecureChannelCredentials()));
    while(true){
        TaskReply reply;
        worker.GetTask(reply);
        if(reply.wait()){
            sleep(2);
            continue;
        }else if(!reply.is_reduce()){
            std::cout << "Received Map Task" << reply.task_id() << std::endl;
            example_map(reply.file());

            worker.FinishTask(reply.job_id(), reply.task_id(), reply.is_reduce(),true);
        }else{
            std::cout << "Received Reduce Task" << reply.task_id() << std::endl;
            worker.FinishTask(reply.job_id(), reply.task_id(), reply.is_reduce(),true);
        }
    }
    return 0;
};