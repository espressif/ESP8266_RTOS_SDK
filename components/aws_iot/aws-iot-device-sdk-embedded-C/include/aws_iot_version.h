/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file aws_iot_version.h
 * @brief Constants defining the release version of the SDK.
 *
 * This file contains constants defining the release version of the SDK.
 * This file is modified by AWS upon release of the SDK and should not be
 * modified by the consumer of the SDK.  The provided samples show example
 * usage of these constants.
 *
 * Versioning of the SDK follows the MAJOR.MINOR.PATCH Semantic Versioning guidelines.
 * @see http://semver.org/
 */
#ifndef SRC_UTILS_AWS_IOT_VERSION_H_
#define SRC_UTILS_AWS_IOT_VERSION_H_

/**
 * @brief MAJOR version, incremented when incompatible API changes are made.
 */
#define VERSION_MAJOR 2
/**
 * @brief MINOR version when functionality is added in a backwards-compatible manner.
 */
#define VERSION_MINOR 2
/**
 * @brief PATCH version when backwards-compatible bug fixes are made.
 */
#define VERSION_PATCH 1
/**
 * @brief TAG is an (optional) tag appended to the version if a more descriptive verion is needed.
 */
#define VERSION_TAG ""

#endif /* SRC_UTILS_AWS_IOT_VERSION_H_ */
