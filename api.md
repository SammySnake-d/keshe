Title: 设备API - 新大陆物联网云平台

URL Source: http://api.nlecloud.com/doc/api/detail?c=devices

Markdown Content:
设备API - 新大陆物联网云平台
===============

[![Image 1](http://api.nlecloud.com/images/nlogo_blue_s.png)开发文档](http://api.nlecloud.com/)

*   [平台概述](http://api.nlecloud.com/doc/zh-CN)
*   [快速入门](http://api.nlecloud.com/doc/zh-CN/quickstart.shtml)
*   [设备接入▾](http://api.nlecloud.com/doc/zh-CN/devicedev/)
    *   [接入实例](http://api.nlecloud.com/doc/zh-CN/devicedev/example.shtml)

*   [应用开发▾](http://api.nlecloud.com/doc/zh-CN/appdev/)
    *   [RESTfulAPI](http://api.nlecloud.com/doc/api/)
    *   [API SDK库](http://api.nlecloud.com/doc/zh-CN/resources_sdk.shtml#%E5%BA%94%E7%94%A8%E5%BC%80%E5%8F%91SDK)
    *   [API在线调试](http://api.nlecloud.com/tool/debugtool)
    *   [应用设计器](http://api.nlecloud.com/doc/zh-CN/appdev/appdesign.shtml)
    *   [应用示范](http://api.nlecloud.com/doc/zh-CN/appdev/example.shtml)

*   [资源下载](http://api.nlecloud.com/doc/zh-CN/resources.shtml)
*   [Q&A](http://api.nlecloud.com/doc/zh-CN/q&a.shtml)

[开发者中心](http://api.nlecloud.com/project)

目录

*   [1. 接口概览](http://api.nlecloud.com/doc/api)
*   [2. 详细列表](javascript:;)
*   [2.1.帐号API](http://api.nlecloud.com/doc/api/detail?c=users)
*   [2.2.项目API](http://api.nlecloud.com/doc/api/detail?c=projects)
*   [2.3.设备API](http://api.nlecloud.com/doc/api/detail?c=devices)
*   [2.4.设备传感器API](http://api.nlecloud.com/doc/api/detail?c=sensors)
*   [2.5.传感数据API](http://api.nlecloud.com/doc/api/detail?c=datas%7Cbindata)
*   [2.6.策略API](http://api.nlecloud.com/doc/api/detail?c=strategys)
*   [2.7.命令API](http://api.nlecloud.com/doc/api/detail?c=cmds)

应用开发●API接口●设备API
================

### 批量查询设备最新数据

请求方式及地址

```
GET 
   http://api.nlecloud.com/Devices/Datas
```

URL请求参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| devIds | string | 设备ID用逗号隔开, 限制100个设备 | Required |

响应参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| ResultObj | Collection of [DeviceSensorDataDTO](javascript:;) |  |  |
| Status | [ResultStatus](javascript:;) | 返回状态 |  |
| StatusCode | integer | 返回的状态码 |  |
| Msg | string | 返回的消息 |  |
| ErrorObj | Object |  |  |

响应示例

{
  "ResultObj": [
    {
      "DeviceID": 1,
      "Name": "sample string 2",
      "Datas": [
        {
          "ApiTag": "sample string 1",
          "Value": {},
          "RecordTime": "sample string 3"
        },
        {
          "ApiTag": "sample string 1",
          "Value": {},
          "RecordTime": "sample string 3"
        }
      ]
    },
    {
      "DeviceID": 1,
      "Name": "sample string 2",
      "Datas": [
        {
          "ApiTag": "sample string 1",
          "Value": {},
          "RecordTime": "sample string 3"
        },
        {
          "ApiTag": "sample string 1",
          "Value": {},
          "RecordTime": "sample string 3"
        }
      ]
    }
  ],
  "Status": 0,
  "StatusCode": 1,
  "Msg": "sample string 2",
  "ErrorObj": {}
}

### 批量查询设备实时模拟数据

请求方式及地址

```
GET 
   http://api.nlecloud.com/Devices/SimulationDatas
```

URL请求参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| devIds | string | 设备ID用逗号隔开, 限制100个设备 | Required |

响应参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| ResultObj | Collection of [SimulationDatasDTO](javascript:;) |  |  |
| Status | [ResultStatus](javascript:;) | 返回状态 |  |
| StatusCode | integer | 返回的状态码 |  |
| Msg | string | 返回的消息 |  |
| ErrorObj | Object |  |  |

响应示例

{
  "ResultObj": [
    {
      "DeviceID": 1,
      "Name": "sample string 2",
      "Floor": "sample string 3",
      "Room": "sample string 4",
      "Datas": [
        {
          "ApiTag": "sample string 1",
          "Value": {}
        },
        {
          "ApiTag": "sample string 1",
          "Value": {}
        }
      ]
    },
    {
      "DeviceID": 1,
      "Name": "sample string 2",
      "Floor": "sample string 3",
      "Room": "sample string 4",
      "Datas": [
        {
          "ApiTag": "sample string 1",
          "Value": {}
        },
        {
          "ApiTag": "sample string 1",
          "Value": {}
        }
      ]
    }
  ],
  "Status": 0,
  "StatusCode": 1,
  "Msg": "sample string 2",
  "ErrorObj": {}
}

### 批量查询设备的在线状态

请求方式及地址

```
GET 
   http://api.nlecloud.com/Devices/Status
```

URL请求参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| devIds | string | 设备ID用逗号隔开, 限制100个设备 | Required |

响应参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| ResultObj | Collection of [OnlineDataDTO](javascript:;) |  |  |
| Status | [ResultStatus](javascript:;) | 返回状态 |  |
| StatusCode | integer | 返回的状态码 |  |
| Msg | string | 返回的消息 |  |
| ErrorObj | Object |  |  |

响应示例

{
  "ResultObj": [
    {
      "DeviceID": 1,
      "Name": "sample string 2",
      "IsOnline": true,
      "LastOnlineIP": "sample string 4",
      "Tag": "sample string 5"
    },
    {
      "DeviceID": 1,
      "Name": "sample string 2",
      "IsOnline": true,
      "LastOnlineIP": "sample string 4",
      "Tag": "sample string 5"
    }
  ],
  "Status": 0,
  "StatusCode": 1,
  "Msg": "sample string 2",
  "ErrorObj": {}
}

### 推送边缘网关设备

请求方式及地址

```
POST 
   http://api.nlecloud.com/Devices/GatewayDeviceFastAdd
```

包体请求参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| GatewayTag | string | 网关标识 |  |
| SecurityKey | string | 网关密钥 |  |
| Sensors | Collection of [Sensor](javascript:;) | 网关设备列表 |  |

请求示例

{
  "GatewayTag": "sample string 1",
  "SecurityKey": "sample string 2",
  "Sensors": [
    {
      "sid": 1,
      "apitag": "sample string 2",
      "name": "sample string 3",
      "slave_ip": "sample string 4",
      "slave_port": 5,
      "user_name": "sample string 6",
      "user_password": "sample string 7",
      "videoStreamPort": 8,
      "videoStreamUrl": "sample string 9",
      "groupTag": "sample string 10",
      "unit": "sample string 11"
    },
    {
      "sid": 1,
      "apitag": "sample string 2",
      "name": "sample string 3",
      "slave_ip": "sample string 4",
      "slave_port": 5,
      "user_name": "sample string 6",
      "user_password": "sample string 7",
      "videoStreamPort": 8,
      "videoStreamUrl": "sample string 9",
      "groupTag": "sample string 10",
      "unit": "sample string 11"
    }
  ]
}

响应参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| ResultObj | integer |  |  |
| Status | [ResultStatus](javascript:;) | 返回状态 |  |
| StatusCode | integer | 返回的状态码 |  |
| Msg | string | 返回的消息 |  |
| ErrorObj | Object |  |  |

响应示例

{
  "ResultObj": 1,
  "Status": 0,
  "StatusCode": 2,
  "Msg": "sample string 3",
  "ErrorObj": {}
}

### 查询单个设备

请求方式及地址

```
GET 
   http://api.nlecloud.com/Devices/{deviceId}
```

URL请求参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| deviceId | integer | 设备ID | Required |

响应参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| ResultObj | [DeviceInfoDTO](javascript:;) |  |  |
| Status | [ResultStatus](javascript:;) | 返回状态 |  |
| StatusCode | integer | 返回的状态码 |  |
| Msg | string | 返回的消息 |  |
| ErrorObj | Object |  |  |

响应示例

{
  "ResultObj": {
    "Sensors": [
      {
        "ApiTag": "sample string 1",
        "Groups": 64,
        "Protocol": 64,
        "Name": "sample string 4",
        "CreateDate": "sample string 5",
        "TransType": 64,
        "DataType": 64,
        "TypeAttrs": {},
        "DeviceID": 9,
        "SensorType": "sample string 10",
        "GroupID": 1,
        "Coordinate": "sample string 11",
        "Value": {},
        "RecordTime": "sample string 13"
      },
      {
        "ApiTag": "sample string 1",
        "Groups": 64,
        "Protocol": 64,
        "Name": "sample string 4",
        "CreateDate": "sample string 5",
        "TransType": 64,
        "DataType": 64,
        "TypeAttrs": {},
        "DeviceID": 9,
        "SensorType": "sample string 10",
        "GroupID": 1,
        "Coordinate": "sample string 11",
        "Value": {},
        "RecordTime": "sample string 13"
      }
    ],
    "DeviceID": 1,
    "Name": "sample string 2",
    "Tag": "sample string 3",
    "SecurityKey": "sample string 4",
    "ProjectID": 5,
    "Protocol": "sample string 6",
    "IsOnline": true,
    "LastOnlineIP": "sample string 8",
    "LastOnlineTime": "sample string 9",
    "Coordinate": "sample string 10",
    "CreateDate": "sample string 11",
    "IsShare": true,
    "IsTrans": true
  },
  "Status": 0,
  "StatusCode": 1,
  "Msg": "sample string 2",
  "ErrorObj": {}
}

### 模糊查询设备

请求方式及地址

```
GET 
   http://api.nlecloud.com/Devices
```

URL请求参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| Keyword | string | 关键字（可选，从id或name字段左匹配） |  |
| DeviceIds | string | 指定设备ID（可选，如“124,34423,2345”，多个用逗号分隔，最多100个） |  |
| Tag | string | 设备标识（可选） |  |
| IsOnline | string | 在线状态（可选，true|false） |  |
| IsShare | string | 数据保密性（可选，true|false） |  |
| ProjectKeyWord | string | 项目ID或纯32位字符的项目标识码（可选） |  |
| PageSize | integer | 指定每页要显示的数据个数，默认20，最多100 |  |
| StartDate | string | 起始时间（可选，包括当天，格式YYYY-MM-DD） |  |
| EndDate | string | 结束时间（可选，包括当天，格式YYYY-MM-DD） |  |
| PageIndex | integer | 指定页码 |  |

响应参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| ResultObj | [ListPagerSetOfDeviceBaseInfoDTO](javascript:;) |  |  |
| Status | [ResultStatus](javascript:;) | 返回状态 |  |
| StatusCode | integer | 返回的状态码 |  |
| Msg | string | 返回的消息 |  |
| ErrorObj | Object |  |  |

响应示例

{
  "ResultObj": {
    "PageSet": [
      {
        "DeviceID": 1,
        "Name": "sample string 2",
        "Tag": "sample string 3",
        "SecurityKey": "sample string 4",
        "ProjectID": 5,
        "Protocol": "sample string 6",
        "IsOnline": true,
        "LastOnlineIP": "sample string 8",
        "LastOnlineTime": "sample string 9",
        "Coordinate": "sample string 10",
        "CreateDate": "sample string 11",
        "IsShare": true,
        "IsTrans": true
      },
      {
        "DeviceID": 1,
        "Name": "sample string 2",
        "Tag": "sample string 3",
        "SecurityKey": "sample string 4",
        "ProjectID": 5,
        "Protocol": "sample string 6",
        "IsOnline": true,
        "LastOnlineIP": "sample string 8",
        "LastOnlineTime": "sample string 9",
        "Coordinate": "sample string 10",
        "CreateDate": "sample string 11",
        "IsShare": true,
        "IsTrans": true
      }
    ],
    "PageCount": 1,
    "PageIndex": 2,
    "PageSize": 3,
    "RecordCount": 4
  },
  "Status": 0,
  "StatusCode": 1,
  "Msg": "sample string 2",
  "ErrorObj": {}
}

### 添加个新设备

请求方式及地址

```
POST 
   http://api.nlecloud.com/Devices
```

包体请求参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| Protocol | byte | 通讯协议（1:TCP 2:MQTT 3:HTTP 5:LWM2M 6:COAP 7:TCP透传 8:MODBUS） | Required |
| IsTrans | boolean | 数据上报状态，true | false（可选，默认为ture） |  |
| ProjectIdOrTag | string | 项目ID（一个数字）或标识码（一个32位字符串） | String length: inclusive between 1 and 32 |
| Name | string | 设备名称（中英文、数字的6到15个字） | RequiredString length: inclusive between 1 and 30 |
| Tag | string | 设备标识（英文、数字或其组合6到30个字符） | RequiredMatching regular expression pattern: ^[a-zA-Z0-9_]{6,30}$ |
| Coordinate | string | 设备座标（可选，格式为经度值, 纬度值） |  |
| DeviceImg | string | 设备头像（可选） |  |
| IsShare | boolean | 数据保密性，true | false（可选，默认为ture） |  |

请求示例

{
  "Protocol": 64,
  "IsTrans": true,
  "ProjectIdOrTag": "sample string 3",
  "Name": "sample string 7",
  "Tag": "sample string 8",
  "Coordinate": "sample string 9",
  "DeviceImg": "sample string 10",
  "IsShare": true,
  "ReturnUrl": "sample string 12",
  "DataToken": "sample string 13"
}

响应参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| ResultObj | integer |  |  |
| Status | [ResultStatus](javascript:;) | 返回状态 |  |
| StatusCode | integer | 返回的状态码 |  |
| Msg | string | 返回的消息 |  |
| ErrorObj | Object |  |  |

响应示例

{
  "ResultObj": 1,
  "Status": 0,
  "StatusCode": 2,
  "Msg": "sample string 3",
  "ErrorObj": {}
}

### 更新某个设备

请求方式及地址

```
PUT 
   http://api.nlecloud.com/Devices/{deviceId}
```

URL请求参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| deviceId | integer | 更新的设备ID | Required |

包体请求参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| Protocol | byte | 通讯协议（1:TCP 2:MQTT 3:HTTP 5:LWM2M 6:COAP 7:TCP透传 8:MODBUS） | Required |
| IsTrans | boolean | 数据上报状态，true | false（可选，默认为ture） |  |
| ProjectIdOrTag | string | 项目ID（一个数字）或标识码（一个32位字符串） | String length: inclusive between 1 and 32 |
| Name | string | 设备名称（中英文、数字的6到15个字） | RequiredString length: inclusive between 1 and 30 |
| Tag | string | 设备标识（英文、数字或其组合6到30个字符） | RequiredMatching regular expression pattern: ^[a-zA-Z0-9_]{6,30}$ |
| Coordinate | string | 设备座标（可选，格式为经度值, 纬度值） |  |
| DeviceImg | string | 设备头像（可选） |  |
| IsShare | boolean | 数据保密性，true | false（可选，默认为ture） |  |

请求示例

{
  "Protocol": 64,
  "IsTrans": true,
  "ProjectIdOrTag": "sample string 3",
  "Name": "sample string 7",
  "Tag": "sample string 8",
  "Coordinate": "sample string 9",
  "DeviceImg": "sample string 10",
  "IsShare": true,
  "ReturnUrl": "sample string 12",
  "DataToken": "sample string 13"
}

响应参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| Status | [ResultStatus](javascript:;) | 返回状态 |  |
| StatusCode | integer | 返回的状态码 |  |
| Msg | string | 返回的消息 |  |
| ErrorObj | Object |  |  |

响应示例

{
  "Status": 0,
  "StatusCode": 1,
  "Msg": "sample string 2",
  "ErrorObj": {}
}

### 删除某个设备

请求方式及地址

```
DELETE 
   http://api.nlecloud.com/Devices/{deviceId}
```

URL请求参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| deviceId | integer | 设备ID | Required |

响应参数

| 参数 | 类型 | 描述 | 其它 |
| --- | --- | --- | --- |
| Status | [ResultStatus](javascript:;) | 返回状态 |  |
| StatusCode | integer | 返回的状态码 |  |
| Msg | string | 返回的消息 |  |
| ErrorObj | Object |  |  |

响应示例

{
  "Status": 0,
  "StatusCode": 1,
  "Msg": "sample string 2",
  "ErrorObj": {}
}