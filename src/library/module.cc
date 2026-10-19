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
 #include <udjat/module.h>
 #include <udjat/agent.h>
 #include <udjat/action.h>
 #include <udjat/tools/variant.h>
 #include <udjat/agent/user.h>
 #include <udjat/tools/user/list.h>
 #include <udjat/module/users.h>
 #include <udjat/tools/properties.h>
 #include <udjat/agent/user.h>
 #include <memory>
 
 using namespace std;

 namespace Udjat {

	Udjat::Module * User::Module::Factory(const char *name, const char *description) {
		auto module = new User::Module{name, description};
		module->autoclean();
		return module;
	}

	User::Module::Module(const char *name, const char *description) : Udjat::Module(name, description), Action::Factory{name} {
		// Get User list singleton.
		debug("Loading module '",name,"'");
		User::List::getInstance();
	};

	User::Module::~Module() {
	}

	std::shared_ptr<Action> User::Module::ActionFactory(const Udjat::Properties &props) const {

		class UserListAction : public Udjat::Action {
		public:
			UserListAction(const Udjat::Properties &props) : Udjat::Action{props} {
			}

			~UserListAction() override {
			}

			int call(Udjat::Request &request, Udjat::Response &response, bool except) {
				return exec(response,except,[&](){

					for(auto session : User::List::getInstance()) {
						auto row = response.append(Variant::ValueMap);
						row["name"] = session->to_string();
						row["remote"] = session->remote();
						row["locked"] = session->locked();
						row["active"] = session->active();
						row["state"] = std::to_string(session->state());
					}

					return 0;
				});
			}

		};

		return std::make_shared<UserListAction>(props);
	}

 }	