//
// Created by Yile Hu on 10/14/24.
//
#include <iostream>
#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"
#include <filesystem>
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
using task::Job;
using task::JobID;
class Client{
public:
    Client(std::shared_ptr<Channel> channel)
            : stub_(MRCoordinator::NewStub(channel)) {};
    Status SubmitJob(Job& j, JobID& reply){
        ClientContext context;
        return stub_->SubmitJob(&context, j, &reply);
    }
private:
    std::unique_ptr<MRCoordinator::Stub> stub_;
};

int main(int argc, char* argv[]){
    absl::ParseCommandLine(argc, argv);
    Client client(grpc::CreateChannel("localhost:50051",
                               grpc::InsecureChannelCredentials()));
    Job j;
    std::string path = std::filesystem::current_path();
    int i = 0;
    for (const auto & entry : std::filesystem::directory_iterator(path)) {
        std::string* s = j.add_file_list();
        *s = entry.path();
        std::cout << "*" << *s << std::endl;
        i++;
    }
    j.set_n_reduce(10);
    JobID jid;
    client.SubmitJob(j, jid);
    std::cout << jid.job_id() << std::endl;
}