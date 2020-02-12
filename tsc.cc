#include <iostream>
//#include <memory>
//#include <thread>
//#include <vector>
#include <string>
#include <unistd.h>
#include <grpc++/grpc++.h>
#include "client.h"
#include "tinysns.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using tinysns::User;
using tinysns::ReplyStatus;
using tinysns::Posting;
using tinysns::NewPosting;
using tinysns::TinySNS;


class Client : public IClient
{
    public:
        Client(const std::string& hname,
               const std::string& uname,
               const std::string& p)
            :hostname(hname), username(uname), port(p)
            {}
    protected:
        virtual int connectTo();
        virtual IReply processCommand(std::string& input);
        virtual void processTimeline();
    private:
        std::string hostname;
        std::string username;
        std::string port;
        
        // You can have an instance of the client stub
        // as a member variable.
    
        std::unique_ptr<TinySNS::Stub> stub_;
};

int main(int argc, char** argv) {

    std::string hostname = "localhost";
    std::string username = "default";
    std::string port = "3010";
    int opt = 0;
    while ((opt = getopt(argc, argv, "h:u:p:")) != -1){
        switch(opt) {
            case 'h':
                hostname = optarg;break;
            case 'u':
                username = optarg;break;
            case 'p':
                port = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    Client myc(hostname, username, port);
    // You MUST invoke "run_client" function to start business logic
    myc.run_client();

    return 0;
}

int Client::connectTo()
{
	// ------------------------------------------------------------
    // In this function, you are supposed to create a stub so that
    // you call service methods in the processCommand/porcessTimeline
    // functions. That is, the stub should be accessible when you want
    // to call any service methods in those functions.
    // I recommend you to have the stub as
    // a member variable in your own Client class.
    // Please refer to gRpc tutorial how to create a stub.
	// ------------------------------------------------------------

    stub_ = TinySNS::NewStub(grpc::CreateChannel(hostname + ":" + port, grpc::InsecureChannelCredentials()));

    ClientContext client_context;

    User current_user; 
    current_user.set_username(username);
    
    ReplyStatus login_status;
    Status status = stub_->Login(&client_context, current_user, &login_status);
    
    if(login_status.status()=="1"){
        std::cout << username <<": successfully logged in" << std::endl;
        return 1;
    } else if(login_status.status()=="0"){
        std::cout << "new user created" << std::endl;
        return 1;
    }else {
        std::cout << "could not establish a connection to host" << std::endl;
        return -1;
    }
}

IReply Client::processCommand(std::string& input)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In this function, you are supposed to parse the given input
    // command and create your own message so that you call an 
    // appropriate service method. The input command will be one
    // of the followings:
	//
	// FOLLOW <username>
	// UNFOLLOW <username>
	// LIST
    // TIMELINE
	//
	// - JOIN/LEAVE and "<username>" are separated by one space.
	// ------------------------------------------------------------
	
    const char* input_copy = input.c_str();
    //std::string string_input = input.copy();
    
    IReply command_reply;
    ReplyStatus status;
    ClientContext command_context;
    User current_user;
    current_user.set_username(username);
    
    if(strncmp(input_copy, "FOLLOW", 6)==0){
        const char* target_name = input.substr(7).c_str();
        tinysns::FollowOp to_follow;
        
        to_follow.set_username(username);
        to_follow.set_follow(target_name);
        
        std::cout << "follow command" << input_copy;
        
        command_reply.grpc_status = stub_->Follow(&command_context, to_follow, &status);
    } else if(strncmp(input_copy, "UNFOLLOW", 8)==0){
        const char* target_name = input.substr(9).c_str();
        
        tinysns::FollowOp to_unfollow;
             
        to_unfollow.set_username(username);
        to_unfollow.set_follow(target_name);
        
        std::cout << (char*)"unfollow command";
        
        command_reply.grpc_status = stub_->Unfollow(&command_context, to_unfollow, &status);
    } else if(strncmp(input_copy, "LIST", 4)==0){
        std::cout << "list command";
    } else if(strncmp(input_copy, "TIMELINE", 8)==0){
        std::cout << "timeline command";
    }

    // Convert to comm iStatus message before returning
    if(status.status() == "0")
        command_reply.comm_status = SUCCESS;
    else if(status.status() == "1")
        command_reply.comm_status = FAILURE_ALREADY_EXISTS;
    else if(status.status() == "2")
        command_reply.comm_status = FAILURE_NOT_EXISTS;
    else if(status.status() == "3")
        command_reply.comm_status = FAILURE_INVALID_USERNAME;
    else if(status.status() == "4")
        command_reply.comm_status = FAILURE_INVALID;
    else if(status.status() == "5")
        command_reply.comm_status = FAILURE_UNKNOWN;
    else
        command_reply.comm_status = SUCCESS;
    
    
    std::cout << status.status();
    
    // ------------------------------------------------------------
	// GUIDE 2:
	// Then, you should create a variable of IReply structure
	// provided by the client.h and initialize it according to
	// the result. Finally you can finish this function by returning
    // the IReply.
	// ------------------------------------------------------------
    
	// ------------------------------------------------------------
    // HINT: How to set the IReply?
    // Suppose you have "Join" service method for JOIN command,
    // IReply can be set as follow:
    // 
    //     // some codes for creating/initializing parameters for
    //     // service method
    //     IReply ire;
    //     grpc::Status status = stub_->Join(&context, /* some parameters */);
    //     ire.grpc_status = status;
    //     if (status.ok()) {
    //         ire.comm_status = SUCCESS;
    //     } else {
    //         ire.comm_status = FAILURE_NOT_EXISTS;
    //     }
    //      
    //      return ire;
    // 
    // IMPORTANT: 
    // For the command "LIST", you should set both "all_users" and 
    // "following_users" member variable of IReply.
	// ------------------------------------------------------------
    
    return command_reply;
}

void Client::processTimeline()
{
	// ------------------------------------------------------------
    // In this function, you are supposed to get into timeline mode.
    // You may need to call a service method to communicate with
    // the server. Use getPostMessage/displayPostMessage functions
    // for both getting and displaying messages in timeline mode.
    // You should use them as you did in hw1.
	// ------------------------------------------------------------

    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    //
    // Once a user enter to timeline mode , there is no way
    // to command mode. You don't have to worry about this situation,
    // and you can terminate the client program by pressing
    // CTRL-C (SIGINT)
	// ------------------------------------------------------------
    
    /*while(true) {
           ClientContext client_context;
        
           if(userInputReady(100000)) {
               NewPost post;
               ReplyStatus rStatus;
               post.set_postfrom(username);
               post.set_posttext(getPostMessage());
               Status stat = stub_->PostTimeline(&context, post, &rStatus);
               if(checkForError(rStatus.stat()) != SUCCESS) {
                   std::cout << "Debug:tsc:processTimeline:Error from server" << std::endl;
               }
           }
           else {
               User user;
               Post post;
               enum IStatus stat;
               std::vector<Post> posts;
               user.set_name(username);
               std::unique_ptr<ClientReader<Post> > reader(stub_->GetTimeline(&context, user));
               //Set a default value for comm_status, since this error should never happen for this command
               bool checkedFistMsg = false;
               //Set default val to success, so a user without anything in their timeline (and would thus skip the loop)
               //can still get to the timeline functionality
               stat = SUCCESS;
               bool newPost = true;
               while(reader->Read(&post)) {
                   //If the default value is still set, check the first passed name for errors
                   if(!checkedFistMsg) {
                       stat = checkForError(post.name());
                       checkedFistMsg = true;
                   }
                   //If all is good, add the posts to the vector for reversing
                   if(newPost && stat == SUCCESS && post.time() > lastPost) {
                       posts.insert(posts.begin(), post);
                   }
                   else {
                       newPost = false;
                   }
               }
               Status s = reader->Finish();
               for(int j = 0; j < posts.size(); j++) {
                   time_t tempTime = posts.at(j).time();
                   displayPostMessage(posts.at(j).name(), posts.at(j).posttext(), tempTime);
                   lastPost = posts.at(j).time();
               }
               if(!s.ok()) {
                   std::cout << "Error in getting update timeline" << std::endl;
               }
           }
       }*/
}
