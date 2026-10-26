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

 #define LOG_DOMAIN "users"

 #include <config.h>
 #include <udjat/tools/user/session.h>
 #include <udjat/tools/user/list.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/configuration.h>
 #include <udjat/tools/threadpool.h>
 #include <vector>

 #include <cstring>
 #include <iostream>

 #ifdef HAVE_SYSTEMD
	#include <systemd/sd-login.h>
 #endif

 #ifdef HAVE_DBUS
	#include <udjat/tools/dbus/connection.h>	
	#include <udjat/tools/dbus/message.h>
 #endif // HAVE_DBUS

 using namespace std;

 namespace Udjat {

	User::List & User::List::getInstance() {
 #ifdef HAVE_DBUS

		class Instance : public User::List {
		private:
			vector<shared_ptr<DBus::Connection::Subscription>> subscriptions;

		public:
			Instance() : User::List{} {

			}

			~Instance() override {

			}

			void activate() override {

				User::List::activate();

				auto &bus = DBus::SystemBus::getInstance();

				if(Config::Value<bool>("user-session","subscribe-prepare-for-sleep",true)) {

					try {
						subscriptions.push_back(bus.subscribe(
							"org.freedesktop.login1.Manager",
							"PrepareForSleep",
							[this](DBus::Message &message) {

								bool flag;
								message.pop(flag);

								if(flag) {
									sleep();
								} else {
									resume();
								}

								return false;

							}
						));
					} catch(const std::exception &e) {

						Logger::String{"Error '",e.what(),"' subscribing to org.freedesktop.login1.Manager.PrepareForSleep"}.error();

					}
				}

				if(Config::Value<bool>("user-session","subscribe-prepare-for-shutdown",true)) {
					try {
						subscriptions.push_back(bus.subscribe(
							"org.freedesktop.login1.Manager",
							"PrepareForShutdown",
							[this](DBus::Message &message) {

								bool flag;
								message.pop(flag);

								if(flag) {
									shutdown();
								}

								return false;

							}
						));
					} catch(const std::exception &e) {
						cerr << "users\tError '" << e.what() << "' subscribing to org.freedesktop.login1.Manager.PrepareForShutdown" << endl;
					}
				}

				// https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.login1.html
				try {

					subscriptions.push_back(bus.subscribe(
						"org.freedesktop.login1.Manager",
						"SessionNew",
						[this](DBus::Message &message) {

							string sid;
							message.pop(sid);

							string path;
							message.pop(path);

							Logger::String{"Session '",sid,"' started on path '",path,"'"}.info();

							ThreadPool::getInstance().push([this,sid](){

								lock_guard<recursive_mutex> lock(guard);

								Session * session = new Session();

								try {

									session->sid = sid;
									session->init();

									session->flags.alive = true;
									session->emit(logon);

									char *state = nullptr;
									if(sd_session_get_state(sid.c_str(), &state) >= 0) {
										session->set(User::StateFactory(state));
										free(state);
									}

								} catch(const std::exception &e) {
									delete session;
									Logger::String{"Unable to initialize session '",sid,"': ",e.what()}.error();
								} catch(...) {
									delete session;
									Logger::String{"Unable to initialize session '",sid,"': Unexpected error"}.error();
								}

							});

							return false;

						}
					));
				} catch(const std::exception &e) {
					Logger::String{"Error '", e.what(), "' subscribing to org.freedesktop.login1.Manager.SessionNew"}.error();
				}

				try {
					subscriptions.push_back(bus.subscribe(
						"org.freedesktop.login1.Manager",
						"SessionRemoved",
						[this](DBus::Message &message) {

							debug("----------------------------------> SessionRemoved");

							string sid;
							message.pop(sid);

							string path;
							message.pop(path);

							Logger::String{"Session '",sid,"' finished on path '",path,"'"}.info();

							ThreadPool::getInstance().push([this,sid](){

								lock_guard<recursive_mutex> lock(guard);

								for(Session *session : sessions) {

									if(strcmp(session->sid.c_str(),sid.c_str())) {
										continue;
									}

									if(session->flags.alive) {

										if(Logger::enabled(Logger::Debug)) {
											Logger::String{
											"Sid=",session->sid,
											" Uid=",session->userid(),
											" System=",session->system(),
											" type=",session->type(),
											" display=",session->display(),
											" remote=",session->remote(),
											" service=",session->service(),
											" class=",session->classname()
											}.write(Logger::Debug,session->name());
										}
										session->emit(logoff);
										session->flags.alive = false;
									}

									session->deinit();
									delete session;

									break;

								}

							});
							return false;

						}
					));
				} catch(const std::exception &e) {
					Logger::String{"Error '",e.what(),"' subscribing to org.freedesktop.login1.Manager.SessionRemoved"}.error();
				}

				try {
					subscriptions.push_back(bus.subscribe(
						"org.freedesktop.login1.Manager",
						"UserNew",
						[this](DBus::Message &message) {

							debug("----------------------------------> UserNew");

							return false;

						}
					));
				} catch(const std::exception &e) {
					Logger::String{"Error '", e.what(), "' subscribing to org.freedesktop.login1.Manager.UserNew"}.error();
				}

				try {
					subscriptions.push_back(bus.subscribe(
						"org.freedesktop.login1.Manager",
						"UserRemoved",
						[this](DBus::Message &message) {

							debug("----------------------------------> UserRemoved");

							return false;

						}
					));
				} catch(const std::exception &e) {
					Logger::String{"Error '",e.what(),"' subscribing to org.freedesktop.login1.Manager.UserRemoved"}.error();
				}

			}

			void deactivate() override {

				auto &bus = DBus::SystemBus::getInstance();
				for(const auto subscription : subscriptions) {
					bus.unsubscribe(subscription);
				}
				subscriptions.clear();

				User::List::deactivate();

			}

		};

		static Instance instance;

 #else
		static User::List instance;
 #endif // HAVE_DBUS

		return instance;
	}

	void User::List::init() noexcept {

		lock_guard<recursive_mutex> lock(guard);
		for(auto session : sessions) {
			Logger::String{"Initializing session @",session->sid}.info();
			session->flags.alive = true;
			session->emit(already_active);
		}

	}

	void User::List::deinit() noexcept {

		lock_guard<recursive_mutex> lock(guard);

		while(!sessions.empty()) {

			Session *session = *sessions.begin();
			Logger::String{"Deinitializing session @",session->sid}.trace("userlist");
			if(session->flags.alive) {
				session->emit(still_active);
				session->flags.alive = false;
			}
			session->deinit();
			delete session;
		}

	}

	void User::List::push_back(User::Agent *agent) {
		lock_guard<recursive_mutex> lock(guard);
		agents.push_back(agent);
	}

	void User::List::remove(User::Agent *agent) {
		lock_guard<recursive_mutex> lock(guard);
		agents.remove(agent);
	}

	void User::List::push_back(User::Session *session) {
		lock_guard<recursive_mutex> lock(guard);
		sessions.push_back(session);
	}

	void User::List::remove(User::Session *session) {
		lock_guard<recursive_mutex> lock(guard);
		sessions.remove(session);
	}

	bool User::List::for_each(const std::function<bool(Session &session)> &callback) {
		lock_guard<recursive_mutex> lock(guard);
		for(auto session : sessions) {
			if(callback(*session)) {
				return true;
			}
		}
		return false;
	}

	bool User::List::for_each(const std::function<bool(User::Agent &agent)> &callback) {
		lock_guard<recursive_mutex> lock(guard);
		for(User::Agent *agent : agents) {
			if(callback(*agent)) {
				return true;
			}
		}
		return false;
	}

	void User::List::emit(const User::Event event, const char *logmessage) noexcept {
		if(logmessage && *logmessage) {
			Logger::String{logmessage}.info();
		}
		ThreadPool::getInstance().push([this,event](){
			lock_guard<recursive_mutex> lock(guard);
			for(auto session : sessions) {
				session->emit(event);
			}
		});

	}

 }
