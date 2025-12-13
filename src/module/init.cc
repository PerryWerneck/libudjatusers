/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2021 Perry Werneck <perry.werneck@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/module/abstract.h>
 #include <udjat/agent/abstract.h>
 #include <udjat/action.h>
 #include <udjat/tools/report.h>
 #include <udjat/module/abstract.h>
 #include <udjat/agent/user.h>
 #include <udjat/tools/user/list.h>
 #include <udjat/module/users.h>

 using namespace std;
 using namespace Udjat;

 /// @brief Register udjat user module.
 Udjat::Module * udjat_module_init() {
	return User::Module::Factory("users","User/Session management module");
 }

namespace Udjat {

	Udjat::Module * User::Module::Factory(const char *name, const char *description) {
		return new User::Module{name, description};
	}

	User::Module::Module(const char *name, const char *description) : Udjat::Module(name, description), Abstract::Agent::Factory(name), Action::Factory{name} {
		// Get User list singleton.
		debug("Loading module '",name,"'");
		User::List::getInstance();
	};

	User::Module::~Module() {
	}

	std::shared_ptr<Abstract::Agent> User::Module::AgentFactory(const XML::Node &node) const {
		return make_shared<User::Agent>(node);
	}

	std::shared_ptr<Action> User::Module::ActionFactory(const XML::Node &node) const {

		class UserListAction : public Udjat::Action {
		public:
			UserListAction(const XML::Node &node) : Udjat::Action{node} {
			}

			~UserListAction() override {
			}

			int call(Udjat::Request &request, Udjat::Response &response, bool except) {
				return exec(response,except,[&](){

					auto &report = response.ReportFactory("name","remote","locked","active","state",nullptr);
					for(auto session : User::List::getInstance()) {
						report.push_back(session->to_string());
						report.push_back(session->remote());
						report.push_back(session->locked());
						report.push_back(session->active());
						report.push_back(std::to_string(session->state()));
					}

					return 0;
				});
			}

		};

		return make_shared<UserListAction>(node);
	}


}	