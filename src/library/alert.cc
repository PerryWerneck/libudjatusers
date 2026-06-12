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
 #include <udjat/tools/intl.h>
 #include <udjat/tools/user/session.h>
 #include <udjat/tools/abstract/object.h>
 #include <udjat/alert.h>
 #include <udjat/agent/user.h>
 #include <udjat/tools/logger.h>

 static const struct {
	bool flag;
	const char *attrname;
 } typenames[] = {
	{ false,	"system"		},
	{ true,		"user"			},
	{ false,	"remote"		},
	{ true,		"local"			},
	{ false,	"locked"		},
	{ false,	"unlocked"		},
	{ false,	"background"	},
	{ false,	"foreground"	},
	{ false,	"active"		},
	{ false,	"inactive"		},
 };

 namespace std {

	string to_string(const Udjat::User::Session::Type type) {

		string rc;
		uint16_t mask = 0x0001;
		for(const auto &attr : typenames) {
			if(type & mask) {
				if(!rc.empty()) {
					rc += ",";
				}
				rc += attr.attrname;

			}
			mask <<= 1;
		}

		return rc;
	}

 }

 namespace Udjat {

	User::Session::Type User::Session::TypeFactory(const XML::Node &node) {

		uint16_t value = 0xFFFF;
		uint16_t mask = 0x01;
		
		for(const auto &type : typenames) {
			auto attr = XML::AttributeFactory(node,String{"allow-on-",type.attrname,"-session"}.c_str());
			if(!attr) {
				attr = XML::AttributeFactory(node,String{"on-",type.attrname,"-session"}.c_str());
			}
			if(!attr) {
				attr = XML::AttributeFactory(node,String{type.attrname,"-session"}.c_str());
			}
			if(attr.as_bool(type.flag)) {
				value |= mask;
			} else {
				value &= (~mask);
			}
			mask <<= 1;
		} 

		return (User::Session::Type) value;
	}

	User::Agent::Proxy::Proxy(const XML::Node &node, const User::Event e, std::shared_ptr<Activatable> a)
		: events{e},filter{User::Session::TypeFactory(node)},activatable{a} {

		if(dynamic_cast<Udjat::Alert *>(activatable.get())) {
			throw std::system_error(ENOTSUP, std::system_category(),"Unable to handle user based alerts");
		}

		classname = String{node,"session-class","user"}.as_quark();
		servicename = String{node,"session-service"}.as_quark();
						
	}

	void User::Agent::Proxy::activate(const User::Session &session, const Abstract::Object &agent) const noexcept {

		auto object = Abstract::Object::merge(&session,&agent,nullptr);
		activatable->activate(*object);
		
	}

 }

